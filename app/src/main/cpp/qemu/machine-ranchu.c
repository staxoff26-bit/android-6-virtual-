#include "machine-ranchu.h"
#include "qemu-logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const QemuMachine s_supported_machines[] = {
    {
        .name = "ranchu",
        .description = "Android 6.0 Marshmallow Ranchu Virtual Machine (Cortex-A15, VirtIO, Goldfish Pipe)",
        .cpu_model = "cortex-a15",
        .max_cpus = 4,
        .is_default = true
    },
    {
        .name = "virt",
        .description = "ARM Virtual I/O Platform (QEMU standard VirtIO board)",
        .cpu_model = "cortex-a15",
        .max_cpus = 8,
        .is_default = false
    },
    {
        .name = "goldfish",
        .description = "Legacy Goldfish Platform (QEMU1 memory-mapped peripherals)",
        .cpu_model = "arm1176",
        .max_cpus = 1,
        .is_default = false
    }
};

static const int s_supported_machines_count = sizeof(s_supported_machines) / sizeof(s_supported_machines[0]);

static void register_device(RanchuBoardState *board, const char *name, const char *type,
                            uint32_t base, uint32_t size, int irq) {
    if (!board || board->device_count >= 12) return;
    DeviceInfo *dev = &board->devices[board->device_count++];
    dev->name = name;
    dev->type = type;
    dev->base_addr = base;
    dev->size = size;
    dev->irq = irq;
    dev->active = true;

    mem_mgr_add_region(&board->mem, name, base, size, false);
    qemu_log(LOG_CAT_PERIPH, "DEV: Périphérique '%s' [%s] mappé à 0x%08X (IRQ: %d)",
             name, type, base, irq);
}

int ranchu_machine_init(RanchuBoardState *board, const char *machine_name, uint32_t ram_mb) {
    if (!board) return -1;
    memset(board, 0, sizeof(RanchuBoardState));

    board->machine_name = (machine_name && strlen(machine_name) > 0) ? machine_name : "ranchu";
    qemu_log(LOG_CAT_MACHINE, "MACH: Initialisation de la machine '%s' (RAM: %u Mo)",
             board->machine_name, ram_mb);

    // 1. Initialize Memory Manager & allocate RAM
    int mem_res = mem_mgr_init(&board->mem, ram_mb);
    if (mem_res != 0) {
        qemu_log(LOG_CAT_ERROR, "MACH: Échec d'allocation mémoire invité");
        return -2;
    }

    // 2. Initialize TCG execution engine
    int tcg_res = tcg_init(&board->tcg);
    if (tcg_res != 0) {
        qemu_log(LOG_CAT_ERROR, "MACH: Échec initialisation TCG");
        mem_mgr_cleanup(&board->mem);
        return -3;
    }

    // 3. Initialize ARMv7 CPU
    cpu_arm_init(&board->cpu, 0, "cortex-a15");

    // 4. Configure Board Peripherals (GIC, UART, VirtIO, Goldfish Pipe)
    board->device_count = 0;
    register_device(board, "arm_gic", "GICv2 (Interrupt Controller)", 0x08000000, 0x00020000, -1);
    register_device(board, "pl011_uart", "UART Serial Console", 0x09000000, 0x00001000, 1);
    register_device(board, "goldfish_pipe", "AOSP Pipe Transport", 0x09010000, 0x00001000, 2);
    register_device(board, "pl031_rtc", "Real Time Clock", 0x09020000, 0x00001000, 3);
    register_device(board, "virtio_mmio_net", "VirtIO Network", 0x0A000000, 0x00000200, 16);
    register_device(board, "virtio_mmio_blk", "VirtIO Block Disk", 0x0A000200, 0x00000200, 17);
    register_device(board, "virtio_mmio_gpu", "VirtIO Graphics Framebuffer", 0x0A000400, 0x00000200, 18);
    register_device(board, "virtio_mmio_input", "VirtIO Touch/Keys Input", 0x0A000600, 0x00000200, 19);

    // 5. Reset CPU to boot vector
    cpu_arm_reset(&board->cpu, RANCHU_RAM_BASE);

    board->initialized = true;
    qemu_log(LOG_CAT_MACHINE, "MACH: Machine '%s' initialisée avec succès (%d périphériques actifs)",
             board->machine_name, board->device_count);

    return 0;
}

void ranchu_machine_cleanup(RanchuBoardState *board) {
    if (!board || !board->initialized) return;

    qemu_log(LOG_CAT_MACHINE, "MACH: Arrêt et libération des composants de '%s'...", board->machine_name);
    tcg_cleanup(&board->tcg);
    mem_mgr_cleanup(&board->mem);
    board->initialized = false;
    qemu_log(LOG_CAT_MACHINE, "MACH: Machine virtuelle détruite proprement");
}

void ranchu_machine_reset(RanchuBoardState *board) {
    if (!board || !board->initialized) return;
    tcg_flush_tb_cache(&board->tcg);
    cpu_arm_reset(&board->cpu, RANCHU_RAM_BASE);
    qemu_log(LOG_CAT_MACHINE, "MACH: Réinitialisation matérielle (Reset) effectuée");
}

int ranchu_machine_step(RanchuBoardState *board, uint32_t cycles) {
    if (!board || !board->initialized) return -1;
    // Advance CPU and TCG translation loop
    tcg_exec_step(&board->tcg, &board->cpu.regs[15], cycles);
    return 0;
}

size_t ranchu_get_available_machines(char *dest, size_t max_bytes) {
    if (!dest || max_bytes == 0) return 0;
    size_t written = 0;
    dest[0] = '\0';

    for (int i = 0; i < s_supported_machines_count; i++) {
        char line[256];
        int n = snprintf(line, sizeof(line), "%s (%s)%s\n",
                         s_supported_machines[i].name,
                         s_supported_machines[i].description,
                         s_supported_machines[i].is_default ? " [PAR DÉFAUT]" : "");
        if (n > 0 && written + (size_t)n < max_bytes) {
            strcat(dest, line);
            written += (size_t)n;
        }
    }
    return written;
}

size_t ranchu_get_detected_devices(RanchuBoardState *board, char *dest, size_t max_bytes) {
    if (!dest || max_bytes == 0) return 0;
    dest[0] = '\0';
    if (!board || !board->initialized) {
        snprintf(dest, max_bytes, "Machine non initialisée");
        return strlen(dest);
    }

    size_t written = 0;
    for (int i = 0; i < board->device_count; i++) {
        DeviceInfo *d = &board->devices[i];
        char line[256];
        int n = snprintf(line, sizeof(line), "• %s [0x%08X-0x%08X] : %s (IRQ: %d)\n",
                         d->name, d->base_addr, d->base_addr + d->size - 1, d->type, d->irq);
        if (n > 0 && written + (size_t)n < max_bytes) {
            strcat(dest, line);
            written += (size_t)n;
        }
    }
    return written;
}
