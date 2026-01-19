#include "asm.h"
#include <stdlib.h>
#include <string.h>

#define OPCODE_OP      0x33
#define OPCODE_OP_IMM  0x13
#define OPCODE_LUI     0x37

static inline uint32_t get_bits(uint32_t v, int hi, int lo) {
    return (v >> lo) & ((1u << (hi - lo + 1)) - 1u);
}

static inline uint32_t make_nop(void) {
    return 0x00000013u; // addi x0, x0, 0
}

void optimize_text_section(Section *text) {
    if (!text || text->buf.size < 4) return;
    size_t count = text->buf.size / 4;
    if (count == 0) return;

    uint32_t *words = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (!words) return;
    memcpy(words, text->buf.data, count * sizeof(uint32_t));

    for (size_t i = 0; i < count; i++) {
        uint32_t insn = words[i];
        uint32_t opcode = get_bits(insn, 6, 0);
        uint32_t rd = get_bits(insn, 11, 7);
        uint32_t f3 = get_bits(insn, 14, 12);
        uint32_t rs1 = get_bits(insn, 19, 15);
        uint32_t rs2 = get_bits(insn, 24, 20);
        uint32_t f7 = get_bits(insn, 31, 25);
        uint32_t imm12 = get_bits(insn, 31, 20);

        // Redundant addi rd, rs1, 0 where rd==rs1 or rd==x0
        if (opcode == OPCODE_OP_IMM && f3 == 0x0 && imm12 == 0) {
            if (rd == rs1 || rd == 0) {
                words[i] = make_nop();
            }
            continue;
        }

        // Redundant add rd, rs1, x0 where rd==rs1 or rd==x0
        if (opcode == OPCODE_OP && f3 == 0x0 && f7 == 0x00 && rs2 == 0) {
            if (rd == rs1 || rd == 0) {
                words[i] = make_nop();
            }
            continue;
        }

        // Fold movhi + addi (lo=0) by nopping the addi
        if (opcode == OPCODE_LUI && i + 1 < count) {
            uint32_t next = words[i + 1];
            uint32_t n_opcode = get_bits(next, 6, 0);
            uint32_t n_rd = get_bits(next, 11, 7);
            uint32_t n_f3 = get_bits(next, 14, 12);
            uint32_t n_rs1 = get_bits(next, 19, 15);
            uint32_t n_imm12 = get_bits(next, 31, 20);
            if (n_opcode == OPCODE_OP_IMM && n_f3 == 0x0 && n_rd == rd && n_rs1 == rd && n_imm12 == 0) {
                words[i + 1] = make_nop();
            }
        }
    }

    memcpy(text->buf.data, words, count * sizeof(uint32_t));
    free(words);
}
