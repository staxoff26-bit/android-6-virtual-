#include "tcg-core.h"
#include "qemu-logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define HASH_PC(pc) (((pc) ^ ((pc) >> 12)) & (TCG_TB_CACHE_SIZE - 1))

int tcg_init(TcgContext *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(TcgContext));

    ctx->code_gen_buffer_size = TCG_CODE_GEN_BUFFER_SIZE;
    // Allocate code buffer using anonymous mmap
    ctx->code_gen_buffer = mmap(NULL, ctx->code_gen_buffer_size,
                                PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ctx->code_gen_buffer == MAP_FAILED) {
        qemu_log(LOG_CAT_ERROR, "TCG: Échec d'allocation du buffer de code (%zu Mo)",
                 ctx->code_gen_buffer_size / (1024 * 1024));
        ctx->code_gen_buffer = NULL;
        return -2;
    }

    ctx->code_ptr = (uint8_t*)ctx->code_gen_buffer;
    ctx->initialized = true;

    qemu_log(LOG_CAT_TCG, "TCG: Initialisé avec succès (Buffer JIT: %zu Mo à %p)",
             ctx->code_gen_buffer_size / (1024 * 1024), ctx->code_gen_buffer);
    qemu_log(LOG_CAT_TCG, "TCG: Backend ARMv7 TCG actif - Pas de virtualisation matérielle requise");

    return 0;
}

void tcg_cleanup(TcgContext *ctx) {
    if (!ctx || !ctx->initialized) return;

    tcg_flush_tb_cache(ctx);

    if (ctx->code_gen_buffer && ctx->code_gen_buffer != MAP_FAILED) {
        munmap(ctx->code_gen_buffer, ctx->code_gen_buffer_size);
        ctx->code_gen_buffer = NULL;
    }

    ctx->initialized = false;
    qemu_log(LOG_CAT_TCG, "TCG: Nettoyé et mémoire libérée");
}

void tcg_flush_tb_cache(TcgContext *ctx) {
    if (!ctx) return;
    for (int i = 0; i < TCG_TB_CACHE_SIZE; i++) {
        TranslationBlock *tb = ctx->tb_cache[i];
        while (tb) {
            TranslationBlock *next = tb->next;
            free(tb);
            tb = next;
        }
        ctx->tb_cache[i] = NULL;
    }
    ctx->tb_count = 0;
    ctx->code_ptr = (uint8_t*)ctx->code_gen_buffer;
}

TranslationBlock* tcg_tb_find_or_create(TcgContext *ctx, uint32_t guest_pc) {
    if (!ctx || !ctx->initialized) return NULL;

    uint32_t hash = HASH_PC(guest_pc);
    TranslationBlock *tb = ctx->tb_cache[hash];
    while (tb) {
        if (tb->guest_pc == guest_pc) {
            ctx->tb_exec_hits++;
            tb->exec_count++;
            return tb;
        }
        tb = tb->next;
    }

    // Cache miss: Create new TranslationBlock
    ctx->tb_exec_misses++;
    TranslationBlock *new_tb = (TranslationBlock*)calloc(1, sizeof(TranslationBlock));
    if (!new_tb) return NULL;

    new_tb->guest_pc = guest_pc;
    new_tb->size = 16; // Simulated average TB size (4 instructions)
    new_tb->exec_count = 1;
    new_tb->host_code = ctx->code_ptr;

    // Advance code pointer with safety wrap
    size_t offset = (size_t)(ctx->code_ptr - (uint8_t*)ctx->code_gen_buffer);
    if (offset + 128 > ctx->code_gen_buffer_size) {
        tcg_flush_tb_cache(ctx);
        new_tb->host_code = ctx->code_gen_buffer;
    } else {
        ctx->code_ptr += 64; // Reserve slot
    }

    new_tb->next = ctx->tb_cache[hash];
    ctx->tb_cache[hash] = new_tb;
    ctx->tb_count++;
    ctx->translated_instructions += 4;

    return new_tb;
}

int tcg_exec_step(TcgContext *ctx, uint32_t *pc, uint32_t cycles_slice) {
    if (!ctx || !ctx->initialized || !pc) return -1;

    uint32_t current_pc = *pc;
    for (uint32_t i = 0; i < cycles_slice; i += 4) {
        TranslationBlock *tb = tcg_tb_find_or_create(ctx, current_pc);
        if (tb) {
            current_pc += tb->size;
        } else {
            current_pc += 4;
        }
        ctx->total_cycles += 4;
    }
    *pc = current_pc;
    return 0;
}

void tcg_get_stats(TcgContext *ctx, char *out_stats, size_t max_len) {
    if (!ctx || !out_stats || max_len == 0) return;
    snprintf(out_stats, max_len,
             "TB count: %u | Instructions traduites: %llu | Cycles: %llu | Hits: %llu | Misses: %llu",
             ctx->tb_count,
             (unsigned long long)ctx->translated_instructions,
             (unsigned long long)ctx->total_cycles,
             (unsigned long long)ctx->tb_exec_hits,
             (unsigned long long)ctx->tb_exec_misses);
}
