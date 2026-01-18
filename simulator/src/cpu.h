#ifndef MINA_CPU_H
#define MINA_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "mem.h"

typedef enum {
    TRAP_NONE = 0,
    TRAP_ILLEGAL_INSN,
    TRAP_FETCH_MISALIGNED,
    TRAP_LOAD_MISALIGNED,
    TRAP_STORE_MISALIGNED,
    TRAP_FETCH_FAULT,
    TRAP_LOAD_FAULT,
    TRAP_STORE_FAULT,
    TRAP_ECALL,
    TRAP_EBREAK,
    TRAP_UNIMPLEMENTED,
    TRAP_CAP_FAULT,
} Trap;

typedef enum {
    MODE_U = 0,
    MODE_S = 1,
    MODE_M = 3,
} PrivMode;

typedef struct {
    uint64_t base;
    uint32_t len;
    uint16_t perm;
    uint16_t otype;
    bool tag;
    bool sealed;
} CapReg;

typedef enum {
    TFMT_FP32 = 0,
    TFMT_FP16 = 1,
    TFMT_BF16 = 2,
    TFMT_FP8_E4M3 = 3,
    TFMT_FP8_E5M2 = 4,
    TFMT_INT8 = 5,
    TFMT_FP4_E2M1 = 6,
} TensorFmt;

typedef struct {
    TensorFmt fmt;
    float v[256];
} TensorReg;

typedef struct {
    uint64_t regs[32];
    uint64_t pc;
    uint64_t steps;
    bool trace;
    bool dump_regs;
    PrivMode mode;

    // CSRs (subset)
    uint64_t mstatus, mie, medeleg, mideleg, mtvec, mip, mscratch, mepc, mcause, mtval;
    uint64_t sstatus, sie, stvec, sip, sscratch, sepc, scause, stval;
    uint64_t cycle, time, instret;

    CapReg caps[32];
    TensorReg tregs[8];

    uint8_t uart_rx[256];
    uint32_t uart_rx_head;
    uint32_t uart_rx_tail;
    uint32_t uart_rx_count;
} Cpu;

void cpu_init(Cpu *c, uint64_t entry);
Trap cpu_step(Cpu *c, Mem *m);
void cpu_dump_regs(const Cpu *c);

#endif
