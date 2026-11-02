#ifndef CPU_ARM_H
#define CPU_ARM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArmV7CpuState {
    uint32_t regs[16];   // R0-R15 (R13=SP, R14=LR, R15=PC)
    uint32_t cpsr;       // Current Program Status Register
    uint32_t spsr;       // Saved Program Status Register
    
    // CP15 System Control Registers
    uint32_t cp15_sctlr; // System Control
    uint32_t cp15_ttbr0; // Translation Table Base 0
    uint32_t cp15_ttbr1; // Translation Table Base 1
    uint32_t cp15_dacr;  // Domain Access Control
    
    // VFP / NEON (32x 64-bit double registers)
    uint64_t vfp_regs[32];
    uint32_t fpscr;
    
    // CPU flags
    bool halted;
    bool irq_pending;
    bool fiq_pending;
    uint32_t cpu_id;
    const char *model_name;
} ArmV7CpuState;

void cpu_arm_init(ArmV7CpuState *cpu, uint32_t cpu_id, const char *model);
void cpu_arm_reset(ArmV7CpuState *cpu, uint32_t entry_point);
void cpu_arm_step(ArmV7CpuState *cpu, uint32_t cycles);
void cpu_arm_raise_irq(ArmV7CpuState *cpu, uint32_t irq_num);
void cpu_arm_lower_irq(ArmV7CpuState *cpu, uint32_t irq_num);

#ifdef __cplusplus
}
#endif

#endif // CPU_ARM_H
