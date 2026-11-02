#ifndef TCG_CORE_H
#define TCG_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCG_CODE_GEN_BUFFER_SIZE  (16 * 1024 * 1024) // 16 MB JIT buffer
#define TCG_MAX_INSNS             512
#define TCG_TB_CACHE_SIZE         4096

typedef struct TranslationBlock {
    uint32_t guest_pc;
    uint32_t size;
    uint32_t flags;
    void *host_code;
    uint32_t exec_count;
    struct TranslationBlock *next;
} TranslationBlock;

typedef struct TcgContext {
    bool initialized;
    void *code_gen_buffer;
    size_t code_gen_buffer_size;
    uint8_t *code_ptr;
    
    // Translation block cache
    TranslationBlock *tb_cache[TCG_TB_CACHE_SIZE];
    uint32_t tb_count;
    
    // Performance & Execution Telemetry
    uint64_t total_cycles;
    uint64_t translated_instructions;
    uint64_t tb_exec_hits;
    uint64_t tb_exec_misses;
} TcgContext;

int  tcg_init(TcgContext *ctx);
void tcg_cleanup(TcgContext *ctx);
void tcg_flush_tb_cache(TcgContext *ctx);
TranslationBlock* tcg_tb_find_or_create(TcgContext *ctx, uint32_t guest_pc);
int  tcg_exec_step(TcgContext *ctx, uint32_t *pc, uint32_t cycles_slice);
void tcg_get_stats(TcgContext *ctx, char *out_stats, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // TCG_CORE_H
