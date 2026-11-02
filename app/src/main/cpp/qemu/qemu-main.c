#include "qemu-main.h"
#include "qemu-version.h"
#include "qemu-logger.h"
#include "machine-ranchu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static QemuEngineState s_state = QEMU_STATE_UNINITIALIZED;
static RanchuBoardState s_board;
static pthread_t s_worker_thread;
static bool s_worker_running = false;
static bool s_stop_requested = false;
static bool s_pause_requested = false;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_state_cond  = PTHREAD_COND_INITIALIZER;

static void* qemu_worker_thread_func(void *arg) {
    (void)arg;
    qemu_log(LOG_CAT_INIT, "THREAD: Boucle d'émulation QEMU démarrée sur un pthread dédié");

    uint64_t step_count = 0;
    while (1) {
        pthread_mutex_lock(&s_state_mutex);
        if (s_stop_requested) {
            pthread_mutex_unlock(&s_state_mutex);
            break;
        }

        while (s_pause_requested && !s_stop_requested) {
            s_state = QEMU_STATE_PAUSED;
            qemu_log(LOG_CAT_INIT, "THREAD: Machine virtuelle en pause");
            pthread_cond_wait(&s_state_cond, &s_state_mutex);
            if (!s_pause_requested && !s_stop_requested) {
                s_state = QEMU_STATE_RUNNING;
                qemu_log(LOG_CAT_INIT, "THREAD: Machine virtuelle reprise");
            }
        }
        pthread_mutex_unlock(&s_state_mutex);

        // Execute a slice of TCG execution steps
        ranchu_machine_step(&s_board, 16);
        step_count++;

        if (step_count % 1000 == 0) {
            char stats[256];
            tcg_get_stats(&s_board.tcg, stats, sizeof(stats));
            qemu_log(LOG_CAT_TCG, "TCG Heartbeat: %s", stats);
        }

        // Cooperative thread yield (1 ms sleep prevents 100% core saturation on Kirin 620)
        usleep(1000);
    }

    qemu_log(LOG_CAT_INIT, "THREAD: Boucle d'émulation QEMU arrêtée proprement");
    return NULL;
}

int qemu_engine_init(void) {
    pthread_mutex_lock(&s_state_mutex);
    if (s_state != QEMU_STATE_UNINITIALIZED && s_state != QEMU_STATE_STOPPED) {
        pthread_mutex_unlock(&s_state_mutex);
        return 0; // Already initialized
    }

    qemu_logger_init();
    qemu_log(LOG_CAT_INIT, "=== INITIALISATION MOTEUR QEMU NATIF ===");
    qemu_log(LOG_CAT_INIT, "Version QEMU : %s (Commit: %s)", QEMU_VERSION, QEMU_SOURCE_COMMIT);
    qemu_log(LOG_CAT_INIT, "Cible d'émulation : %s | Machine : %s | CPU : %s",
             QEMU_TARGET_ARCH, QEMU_TARGET_MACHINE, QEMU_CPU_MODEL);
    qemu_log(LOG_CAT_INIT, "Moteur de traduction : %s", QEMU_TCG_BACKEND);
    qemu_log(LOG_CAT_INIT, "Cible NDK Bionic : API %d (armeabi-v7a)", QEMU_NDK_TARGET_API);

    s_state = QEMU_STATE_INITIALIZED;
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

int qemu_vm_init(const char *machine_name, int ram_mb) {
    pthread_mutex_lock(&s_state_mutex);
    if (s_state == QEMU_STATE_RUNNING || s_state == QEMU_STATE_PAUSED) {
        qemu_log(LOG_CAT_WARN, "INIT_VM: Impossible d'initialiser pendant que la VM s'exécute");
        pthread_mutex_unlock(&s_state_mutex);
        return -1;
    }

    if (s_board.initialized) {
        ranchu_machine_cleanup(&s_board);
    }

    uint32_t ram = (ram_mb > 0) ? (uint32_t)ram_mb : 512;
    int res = ranchu_machine_init(&s_board, machine_name, ram);
    if (res != 0) {
        s_state = QEMU_STATE_ERROR;
        pthread_mutex_unlock(&s_state_mutex);
        return res;
    }

    s_state = QEMU_STATE_INITIALIZED;
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

int qemu_vm_start(void) {
    pthread_mutex_lock(&s_state_mutex);
    if (s_state == QEMU_STATE_RUNNING) {
        pthread_mutex_unlock(&s_state_mutex);
        return 0;
    }

    if (!s_board.initialized) {
        qemu_log(LOG_CAT_WARN, "START: VM non configurée, initialisation automatique sur ranchu (512 Mo)...");
        ranchu_machine_init(&s_board, "ranchu", 512);
    }

    s_stop_requested = false;
    s_pause_requested = false;

    if (!s_worker_running) {
        int err = pthread_create(&s_worker_thread, NULL, qemu_worker_thread_func, NULL);
        if (err != 0) {
            qemu_log(LOG_CAT_ERROR, "START: Impossible de créer le thread QEMU (err=%d)", err);
            s_state = QEMU_STATE_ERROR;
            pthread_mutex_unlock(&s_state_mutex);
            return -1;
        }
        s_worker_running = true;
    }

    s_state = QEMU_STATE_RUNNING;
    qemu_log(LOG_CAT_INIT, "START: Moteur QEMU démarré avec succès en arrière-plan");
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

int qemu_vm_pause(void) {
    pthread_mutex_lock(&s_state_mutex);
    if (s_state != QEMU_STATE_RUNNING) {
        pthread_mutex_unlock(&s_state_mutex);
        return -1;
    }
    s_pause_requested = true;
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

int qemu_vm_resume(void) {
    pthread_mutex_lock(&s_state_mutex);
    if (s_state != QEMU_STATE_PAUSED) {
        pthread_mutex_unlock(&s_state_mutex);
        return -1;
    }
    s_pause_requested = false;
    pthread_cond_signal(&s_state_cond);
    s_state = QEMU_STATE_RUNNING;
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

int qemu_vm_stop(void) {
    pthread_mutex_lock(&s_state_mutex);
    if (!s_worker_running && s_state != QEMU_STATE_RUNNING && s_state != QEMU_STATE_PAUSED) {
        pthread_mutex_unlock(&s_state_mutex);
        return 0;
    }

    qemu_log(LOG_CAT_INIT, "STOP: Arrêt demandé de la machine virtuelle...");
    s_stop_requested = true;
    s_pause_requested = false;
    pthread_cond_signal(&s_state_cond);
    pthread_mutex_unlock(&s_state_mutex);

    if (s_worker_running) {
        pthread_join(s_worker_thread, NULL);
        s_worker_running = false;
    }

    pthread_mutex_lock(&s_state_mutex);
    ranchu_machine_cleanup(&s_board);
    s_state = QEMU_STATE_STOPPED;
    qemu_log(LOG_CAT_INIT, "STOP: Machine virtuelle arrêtée avec succès. Ressources libérées.");
    pthread_mutex_unlock(&s_state_mutex);
    return 0;
}

QemuEngineState qemu_get_state(void) {
    return s_state;
}

const char* qemu_get_state_str(void) {
    switch (s_state) {
        case QEMU_STATE_UNINITIALIZED: return "NON INITIALISÉ";
        case QEMU_STATE_INITIALIZED:   return "INITIALISÉ / PRÊT";
        case QEMU_STATE_RUNNING:       return "DÉMARRÉ (EN COURS D'ÉMULATION)";
        case QEMU_STATE_PAUSED:        return "EN PAUSE";
        case QEMU_STATE_STOPPED:       return "ARRÊTÉ";
        case QEMU_STATE_ERROR:         return "ERREUR";
        default:                       return "INCONNU";
    }
}

const char* qemu_get_version(void) {
    return QEMU_VERSION;
}

size_t qemu_get_logs(char *dest, size_t max_bytes) {
    return qemu_logger_get_recent_logs(dest, max_bytes);
}

size_t qemu_get_available_machines_str(char *dest, size_t max_bytes) {
    return ranchu_get_available_machines(dest, max_bytes);
}

size_t qemu_get_detected_devices_str(char *dest, size_t max_bytes) {
    return ranchu_get_detected_devices(&s_board, dest, max_bytes);
}
