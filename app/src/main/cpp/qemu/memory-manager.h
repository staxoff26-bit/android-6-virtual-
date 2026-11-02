#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RANCHU_RAM_BASE   0x40000000U // 1 GB mark

typedef struct MemoryRegion {
    const char *name;
    uint32_t base_addr;
    uint32_t size;
    uint8_t *host_ptr;
    bool is_ram;
    struct MemoryRegion *next;
} MemoryRegion;

typedef struct QemuMemoryManager {
    bool initialized;
    uint32_t ram_size_mb;
    uint8_t *ram_host_ptr;
    size_t ram_bytes;
    MemoryRegion *regions;
} QemuMemoryManager;

int  mem_mgr_init(QemuMemoryManager *mgr, uint32_t ram_mb);
void mem_mgr_cleanup(QemuMemoryManager *mgr);
int  mem_mgr_add_region(QemuMemoryManager *mgr, const char *name, uint32_t base, uint32_t size, bool is_ram);
uint8_t* mem_mgr_translate_addr(QemuMemoryManager *mgr, uint32_t guest_addr);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_MANAGER_H
