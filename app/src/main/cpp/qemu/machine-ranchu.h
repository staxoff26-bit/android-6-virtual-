#ifndef MACHINE_RANCHU_H
#define MACHINE_RANCHU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cpu-arm.h"
#include "memory-manager.h"
#include "tcg-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DeviceInfo {
    const char *name;
    const char *type;
    uint32_t base_addr;
    uint32_t size;
    int irq;
    bool active;
} DeviceInfo;

typedef struct QemuMachine {
    const char *name;
    const char *description;
    const char *cpu_model;
    uint32_t max_cpus;
    bool is_default;
} QemuMachine;

typedef struct RanchuBoardState {
    bool initialized;
    const char *machine_name;
    ArmV7CpuState cpu;
    QemuMemoryManager mem;
    TcgContext tcg;
    
    // Peripherals
    DeviceInfo devices[12];
    int device_count;
} RanchuBoardState;

int  ranchu_machine_init(RanchuBoardState *board, const char *machine_name, uint32_t ram_mb);
void ranchu_machine_cleanup(RanchuBoardState *board);
void ranchu_machine_reset(RanchuBoardState *board);
int  ranchu_machine_step(RanchuBoardState *board, uint32_t cycles);

size_t ranchu_get_available_machines(char *dest, size_t max_bytes);
size_t ranchu_get_detected_devices(RanchuBoardState *board, char *dest, size_t max_bytes);

#ifdef __cplusplus
}
#endif

#endif // MACHINE_RANCHU_H
