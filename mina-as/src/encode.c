#include "asm.h"

uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    return (uimm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    uint32_t imm_hi = (uimm >> 5) & 0x7F;
    uint32_t imm_lo = uimm & 0x1F;
    return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_lo << 7) | opcode;
}

uint32_t encode_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0x1FFF);
    uint32_t bit12 = (uimm >> 12) & 1;
    uint32_t bit11 = (uimm >> 11) & 1;
    uint32_t bits10_5 = (uimm >> 5) & 0x3F;
    uint32_t bits4_1 = (uimm >> 1) & 0xF;
    return (bit12 << 31) | (bits10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (bits4_1 << 8) | (bit11 << 7) | opcode;
}

uint32_t encode_u(int32_t imm20, uint32_t rd, uint32_t opcode) {
    return ((uint32_t)imm20 << 12) | (rd << 7) | opcode;
}

uint32_t encode_j(int32_t imm, uint32_t rd, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0x1FFFFF);
    uint32_t bit20 = (uimm >> 20) & 1;
    uint32_t bits10_1 = (uimm >> 1) & 0x3FF;
    uint32_t bit11 = (uimm >> 11) & 1;
    uint32_t bits19_12 = (uimm >> 12) & 0xFF;
    return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) | (rd << 7) | opcode;
}
