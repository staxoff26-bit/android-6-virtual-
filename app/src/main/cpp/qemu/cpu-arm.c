#include "cpu-arm.h"
#include "qemu-logger.h"
#include <string.h>

void cpu_arm_init(ArmV7CpuState *cpu, uint32_t cpu_id, const char *model) {
    if (!cpu) return;
    memset(cpu, 0, sizeof(ArmV7CpuState));
    cpu->cpu_id = cpu_id;
    cpu->model_name = model ? model : "cortex-a15";
    cpu->halted = false;

    // ARM CPSR: Supervisor mode (0x13), IRQ/FIQ disabled initially (0xc0)
    cpu->cpsr = 0x000001D3;
    cpu->cp15_sctlr = 0x00C50078; // Reset value for Cortex-A15

    qemu_log(LOG_CAT_INIT, "CPU: Core #%u configuré en modèle '%s' (ARMv7-A, NEON/VFPv4)",
             cpu->cpu_id, cpu->model_name);
}

void cpu_arm_reset(ArmV7CpuState *cpu, uint32_t entry_point) {
    if (!cpu) return;
    for (int i = 0; i < 15; i++) {
        cpu->regs[i] = 0;
    }
    // R15 (PC) initialized to entry point
    cpu->regs[15] = entry_point;
    // R13 (SP) set to top of initial boot RAM
    cpu->regs[13] = 0x47FFFFF0;
    cpu->cpsr = 0x000001D3;
    cpu->halted = false;
    cpu->irq_pending = false;

    qemu_log(LOG_CAT_INIT, "CPU: Core #%u reset effectué - PC = 0x%08X, SP = 0x%08X",
             cpu->cpu_id, cpu->regs[15], cpu->regs[13]);
}

void cpu_arm_step(ArmV7CpuState *cpu, uint32_t cycles) {
    if (!cpu || cpu->halted) return;
    // Simulated architectural instruction increment
    cpu->regs[15] += (cycles * 4);
}

void cpu_arm_raise_irq(ArmV7CpuState *cpu, uint32_t irq_num) {
    if (!cpu) return;
    cpu->irq_pending = true;
    qemu_log(LOG_CAT_PERIPH, "CPU: Interruption IRQ #%u reçue sur Core #%u", irq_num, cpu->cpu_id);
}

void cpu_arm_lower_irq(ArmV7CpuState *cpu, uint32_t irq_num) {
    if (!cpu) return;
    cpu->irq_pending = false;
}
