#ifndef QEMU_MAIN_H
#define QEMU_MAIN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QEMU_STATE_UNINITIALIZED = 0,
    QEMU_STATE_INITIALIZED   = 1,
    QEMU_STATE_RUNNING       = 2,
    QEMU_STATE_PAUSED        = 3,
    QEMU_STATE_STOPPED       = 4,
    QEMU_STATE_ERROR         = 5
} QemuEngineState;

int  qemu_engine_init(void);
int  qemu_vm_init(const char *machine_name, int ram_mb);
int  qemu_vm_start(void);
int  qemu_vm_pause(void);
int  qemu_vm_resume(void);
int  qemu_vm_stop(void);

QemuEngineState qemu_get_state(void);
const char*     qemu_get_state_str(void);
const char*     qemu_get_version(void);

size_t qemu_get_logs(char *dest, size_t max_bytes);
size_t qemu_get_available_machines_str(char *dest, size_t max_bytes);
size_t qemu_get_detected_devices_str(char *dest, size_t max_bytes);

#ifdef __cplusplus
}
#endif

#endif // QEMU_MAIN_H
