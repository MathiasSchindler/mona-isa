#ifndef MINA_ISA_H
#define MINA_ISA_H

#include <stdint.h>

// Opcodes
enum {
    OP_OP       = 0x33,
    OP_OPIMM    = 0x13,
    OP_LOAD     = 0x03,
    OP_STORE    = 0x23,
    OP_BRANCH   = 0x63,
    OP_JAL      = 0x6F,
    OP_JALR     = 0x67,
    OP_MOVHI    = 0x37,
    OP_MOVPC    = 0x17,
    OP_SYSTEM   = 0x73,
    OP_FENCE    = 0x0F,
    OP_AMO      = 0x2F,
    OP_CAP      = 0x2B,
    OP_TENSOR   = 0x5B,
};

static inline uint32_t get_bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1u);
}

static inline int64_t sign_extend(uint64_t v, int bits) {
    uint64_t m = 1ull << (bits - 1);
    return (int64_t)((v ^ m) - m);
}

#endif
