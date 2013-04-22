/* Counts the number of dynamic div instruction for which the
 * divisor is a power of 2 (these are cases where div could be
 * strength reduced to a simple shift).  Demonstrates callout
 * based profiling with live operand values. */

#include "dr_api.h"

#include <stdint.h>

#define NULL_TERMINATE(buf) buf[(sizeof(buf)/sizeof(buf[0])) - 1] = '\0'

static dr_emit_flags_t bb_event(void *drcontext, void *tag, instrlist_t *bb,
                                bool for_trace, bool translating);
static void exit_event(void);

static uint64_t total_count = 0,     total_count_exec = 0;
static uint64_t cbr_count = 0,       cbr_count_exec = 0;
static uint64_t fp_count = 0,        fp_count_exec = 0;
static uint64_t mem_r_count = 0,     mem_r_count_exec = 0;
static uint64_t mem_w_count = 0,     mem_w_count_exec = 0;

static void *count_mutex; /* for multithread support */

DR_EXPORT void
dr_init(client_id_t id)
{
    dr_register_exit_event(exit_event);
    dr_register_bb_event(bb_event);
    count_mutex = dr_mutex_create();
}

static void
exit_event(void)
{
    fprintf(stderr, "total-count=%llu\n", total_count);
    fprintf(stderr, "cbr-count=%llu\n", cbr_count);
    fprintf(stderr, "cbr-count-ratio=%.6f\n", 1.0f * cbr_count / total_count);
    fprintf(stderr, "fp-count=%llu\n", fp_count);
    fprintf(stderr, "fp-count-ratio=%.6f\n", 1.0f * fp_count / total_count);
    fprintf(stderr, "mem-r-count=%llu\n", mem_r_count);
    fprintf(stderr, "mem-r-count-ratio=%.6f\n", 1.0f * mem_r_count / total_count);
    fprintf(stderr, "mem-w-count=%llu\n", mem_w_count);
    fprintf(stderr, "mem-w-count-ratio=%.6f\n", 1.0f * mem_w_count / total_count);
    fprintf(stderr, "mem-count=%llu\n", mem_r_count+mem_w_count);
    fprintf(stderr, "mem-count-ratio=%.6f\n", 1.0f * (mem_r_count+mem_w_count) / total_count);

    fprintf(stderr, "total-count-exec=%llu\n", total_count_exec);
    fprintf(stderr, "cbr-count-exec=%llu\n", cbr_count_exec);
    fprintf(stderr, "cbr-count-exec-ratio=%.6f\n", 1.0f * cbr_count_exec / total_count_exec);
    fprintf(stderr, "fp-count-exec=%llu\n", fp_count_exec);
    fprintf(stderr, "fp-count-exec-ratio=%.6f\n", 1.0f * fp_count_exec / total_count_exec);
    fprintf(stderr, "mem-r-count-exec=%llu\n", mem_r_count_exec);
    fprintf(stderr, "mem-r-count-exec-ratio=%.6f\n", 1.0f * mem_r_count_exec / total_count_exec);
    fprintf(stderr, "mem-w-count-exec=%llu\n", mem_w_count_exec);
    fprintf(stderr, "mem-w-count-exec-ratio=%.6f\n", 1.0f * mem_w_count_exec / total_count_exec);
    fprintf(stderr, "mem-count-exec=%llu\n", mem_r_count_exec+mem_w_count_exec);
    fprintf(stderr, "mem-count-exec-ratio=%.6f\n", 1.0f * (mem_r_count_exec+mem_w_count_exec) / total_count_exec);

    dr_mutex_destroy(count_mutex);
}

static void
callback(int bb_total, int bb_cbr, int bb_fp, int bb_mem_r, int bb_mem_w)
{
    dr_mutex_lock(count_mutex);

    total_count_exec    += bb_total;
    cbr_count_exec      += bb_cbr;
    fp_count_exec       += bb_fp;
    mem_r_count_exec    += bb_mem_r;
    mem_w_count_exec    += bb_mem_w;

    dr_mutex_unlock(count_mutex);
}

static dr_emit_flags_t
bb_event(void* drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
{
    int opcode;
    uint bb_total = 0, bb_cbr = 0, bb_fp = 0, bb_mem_r = 0, bb_mem_w = 0;

    for (instr_t* instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr))
    {
        ++bb_total;

        opcode = instr_get_opcode(instr);

        if (instr_is_cbr(instr))
        {
            ++bb_cbr;
        }

        if (instr_is_floating(instr))
        {
            ++bb_fp;
        }

        if (instr_reads_memory(instr))
        {
            ++bb_mem_r;
        }

        if (instr_writes_memory(instr))
        {
            ++bb_mem_w;
        }
    }

    total_count     += bb_total;
    cbr_count       += bb_cbr;
    fp_count        += bb_fp;
    mem_r_count     += bb_mem_r;
    mem_w_count     += bb_mem_w;

    dr_insert_clean_call(drcontext, bb, instrlist_first(bb), (void *)callback,
                         false /*no fp save*/, 5,
                         OPND_CREATE_INT32(bb_total),
                         OPND_CREATE_INT32(bb_cbr),
                         OPND_CREATE_INT32(bb_fp),
                         OPND_CREATE_INT32(bb_mem_r),
                         OPND_CREATE_INT32(bb_mem_w));

    return DR_EMIT_DEFAULT;
}

