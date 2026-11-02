#include "memory-manager.h"
#include "qemu-logger.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int mem_mgr_init(QemuMemoryManager *mgr, uint32_t ram_mb) {
    if (!mgr) return -1;
    memset(mgr, 0, sizeof(QemuMemoryManager));

    mgr->ram_size_mb = ram_mb;
    mgr->ram_bytes = (size_t)ram_mb * 1024 * 1024;

    // Allocate physical RAM for VM using mmap anonymous
    mgr->ram_host_ptr = (uint8_t*)mmap(NULL, mgr->ram_bytes,
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mgr->ram_host_ptr == MAP_FAILED) {
        qemu_log(LOG_CAT_ERROR, "MEM: Échec mmap pour %u Mo de RAM invité", ram_mb);
        mgr->ram_host_ptr = NULL;
        return -2;
    }

    // Touch first and last page to verify allocation
    mgr->ram_host_ptr[0] = 0;
    mgr->ram_host_ptr[mgr->ram_bytes - 1] = 0;

    mgr->initialized = true;

    // Register System RAM region
    mem_mgr_add_region(mgr, "ranchu.ram", RANCHU_RAM_BASE, (uint32_t)mgr->ram_bytes, true);

    qemu_log(LOG_CAT_INIT, "MEM: RAM invité allouée : %u Mo à [0x%08X - 0x%08X] (Host buffer: %p)",
             ram_mb, RANCHU_RAM_BASE, RANCHU_RAM_BASE + (uint32_t)mgr->ram_bytes - 1, mgr->ram_host_ptr);

    return 0;
}

void mem_mgr_cleanup(QemuMemoryManager *mgr) {
    if (!mgr || !mgr->initialized) return;

    // Free region list
    MemoryRegion *curr = mgr->regions;
    while (curr) {
        MemoryRegion *next = curr->next;
        free(curr);
        curr = next;
    }
    mgr->regions = NULL;

    if (mgr->ram_host_ptr && mgr->ram_host_ptr != MAP_FAILED) {
        munmap(mgr->ram_host_ptr, mgr->ram_bytes);
        mgr->ram_host_ptr = NULL;
    }

    mgr->initialized = false;
    qemu_log(LOG_CAT_INIT, "MEM: RAM invité (%u Mo) libérée", mgr->ram_size_mb);
}

int mem_mgr_add_region(QemuMemoryManager *mgr, const char *name, uint32_t base, uint32_t size, bool is_ram) {
    if (!mgr) return -1;
    MemoryRegion *reg = (MemoryRegion*)calloc(1, sizeof(MemoryRegion));
    if (!reg) return -2;

    reg->name = name;
    reg->base_addr = base;
    reg->size = size;
    reg->is_ram = is_ram;
    if (is_ram) {
        reg->host_ptr = mgr->ram_host_ptr;
    } else {
        reg->host_ptr = NULL; // MMIO Trap
    }

    reg->next = mgr->regions;
    mgr->regions = reg;

    qemu_log(LOG_CAT_INIT, "MEM: Région '%s' enregistrée [0x%08X - 0x%08X] (%s)",
             name, base, base + size - 1, is_ram ? "RAM" : "MMIO");
    return 0;
}

uint8_t* mem_mgr_translate_addr(QemuMemoryManager *mgr, uint32_t guest_addr) {
    if (!mgr || !mgr->initialized) return NULL;

    if (guest_addr >= RANCHU_RAM_BASE && guest_addr < (RANCHU_RAM_BASE + mgr->ram_bytes)) {
        uint32_t offset = guest_addr - RANCHU_RAM_BASE;
        return mgr->ram_host_ptr + offset;
    }
    return NULL;
}
