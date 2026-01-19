#include "asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int is_section_directive(char *tokens[], int count, SectionKind *out) {
    if (count == 0 || tokens[0][0] != '.') return 0;
    if (strcmp(tokens[0], ".text") == 0) { *out = SEC_TEXT; return 1; }
    if (strcmp(tokens[0], ".data") == 0) { *out = SEC_DATA; return 1; }
    if (strcmp(tokens[0], ".rodata") == 0) { *out = SEC_DATA; return 1; }
    if (strcmp(tokens[0], ".bss") == 0) { *out = SEC_BSS; return 1; }
    if (strcmp(tokens[0], ".section") == 0 && count >= 2 && section_from_name(tokens[1], out)) return 1;
    return 0;
}

static int is_unconditional_jump(char *tokens[], int count) {
    if (count == 0) return 0;
    const char *op = tokens[0];
    if (strcmp(op, "j") == 0 || strcmp(op, "jr") == 0 || strcmp(op, "ret") == 0) return 1;
    if (strcmp(op, "jal") == 0 && count >= 3) {
        return parse_reg(tokens[1]) == 0;
    }
    if (strcmp(op, "jalr") == 0 && count >= 4) {
        return parse_reg(tokens[1]) == 0;
    }
    return 0;
}

int emit_directive(Section *sec, SectionKind kind, char *tokens[], int count) {
    if (count == 0) return 1;
    if (strcmp(tokens[0], ".org") == 0) {
        if (count < 2) return 0;
        int64_t v; if (!parse_int(tokens[1], &v)) return 0;
        if ((uint64_t)v < sec->base) return 0;
        uint64_t target = (uint64_t)v - sec->base;
        if (kind == SEC_BSS) {
            sec->pc = target;
            return 1;
        }
        if (target > sec->buf.size) {
            size_t pad = (size_t)(target - sec->buf.size);
            for (size_t i = 0; i < pad; i++) buf_write_u8(&sec->buf, 0);
        }
        sec->pc = target;
        return 1;
    }
    if (strcmp(tokens[0], ".align") == 0) {
        if (count < 2) return 0;
        int64_t v; if (!parse_int(tokens[1], &v)) return 0;
        if (v < 0) return 0;
        uint64_t align = (v >= 63) ? (1ull << 63) : (1ull << v);
        uint64_t next = align_up(sec->pc, align);
        while (sec->pc < next) {
            if (kind != SEC_BSS) buf_write_u8(&sec->buf, 0);
            sec->pc++;
        }
        return 1;
    }
    if (strcmp(tokens[0], ".globl") == 0 || strcmp(tokens[0], ".global") == 0 ||
        strcmp(tokens[0], ".file") == 0 || strcmp(tokens[0], ".loc") == 0 ||
        strcmp(tokens[0], ".type") == 0 || strcmp(tokens[0], ".size") == 0) {
        return 1;
    }
    if (strcmp(tokens[0], ".byte") == 0 && count >= 2) {
        for (int i = 1; i < count; i++) {
            int64_t v; if (!parse_int(tokens[i], &v)) return 0;
            if (kind != SEC_BSS) buf_write_u8(&sec->buf, (uint8_t)v);
            sec->pc++;
        }
        return 1;
    }
    if (strcmp(tokens[0], ".half") == 0 && count >= 2) {
        for (int i = 1; i < count; i++) {
            int64_t v; if (!parse_int(tokens[i], &v)) return 0;
            if (kind != SEC_BSS) buf_write_u16(&sec->buf, (uint16_t)v);
            sec->pc += 2;
        }
        return 1;
    }
    if (strcmp(tokens[0], ".word") == 0 && count >= 2) {
        for (int i = 1; i < count; i++) {
            int64_t v; if (!parse_int(tokens[i], &v)) return 0;
            if (kind != SEC_BSS) buf_write_u32(&sec->buf, (uint32_t)v);
            sec->pc += 4;
        }
        return 1;
    }
    if (strcmp(tokens[0], ".dword") == 0 && count >= 2) {
        for (int i = 1; i < count; i++) {
            int64_t v; if (!parse_int(tokens[i], &v)) return 0;
            if (kind != SEC_BSS) buf_write_u64(&sec->buf, (uint64_t)v);
            sec->pc += 8;
        }
        return 1;
    }
    if (strcmp(tokens[0], ".ascii") == 0 || strcmp(tokens[0], ".asciz") == 0) {
        if (count < 2) return 0;
        if (kind == SEC_BSS) return 0;
        const char *s = tokens[1];
        if (*s == '"') {
            s++;
            size_t len = strlen(s);
            if (len > 0 && s[len - 1] == '"') len--;
            for (size_t i = 0; i < len; i++) { buf_write_u8(&sec->buf, (uint8_t)s[i]); sec->pc++; }
            if (strcmp(tokens[0], ".asciz") == 0) { buf_write_u8(&sec->buf, 0); sec->pc++; }
            return 1;
        }
        return 0;
    }
    return 0;
}

int assemble_line(Section *sec, SectionKind kind, char *tokens[], int count, const LabelTable *labels) {
    if (count == 0) return 1;
    if (tokens[0][0] == '.') return emit_directive(sec, kind, tokens, count);
    if (kind != SEC_TEXT) return 0;

    uint64_t abs_pc = sec->base + sec->pc;
    const char *op = tokens[0];

    // Pseudo-instructions
    if (strcmp(op, "nop") == 0) {
        buf_write_u32(&sec->buf, encode_r(0x00, 0, 0, 0x0, 0, 0x33)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "mov") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        buf_write_u32(&sec->buf, encode_r(0x00, 0, rs1, 0x0, rd, 0x33)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "not") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        buf_write_u32(&sec->buf, encode_i(-1, rs1, 0x4, rd, 0x13)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "ret") == 0) {
        buf_write_u32(&sec->buf, encode_i(0, 31, 0x0, 0, 0x67)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "j") == 0 && count >= 2) {
        int64_t imm = resolve_imm(tokens[1], labels, abs_pc, 1);
        buf_write_u32(&sec->buf, encode_j((int32_t)imm, 0, 0x6F)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "jr") == 0 && count >= 2) {
        int rs1 = parse_reg(tokens[1]);
        buf_write_u32(&sec->buf, encode_i(0, rs1, 0x0, 0, 0x67)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "li") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int64_t imm = 0;
        if (parse_int(tokens[2], &imm)) {
            if (imm >= -2048 && imm <= 2047) {
                buf_write_u32(&sec->buf, encode_i((int32_t)imm, 0, 0x0, rd, 0x13));
                sec->pc += 4;
                return 1;
            }
        } else {
            imm = resolve_imm(tokens[2], labels, abs_pc, 0);
        }
        int64_t hi = (imm + 0x800) >> 12;
        int64_t lo = imm - (hi << 12);
        buf_write_u32(&sec->buf, encode_u((int32_t)hi, rd, 0x37));
        buf_write_u32(&sec->buf, encode_i((int32_t)lo, rd, 0x0, rd, 0x13));
        sec->pc += 8;
        return 1;
    }

    if (strcmp(op, "ecall") == 0) { buf_write_u32(&sec->buf, encode_i(0x000, 0, 0, 0, 0x73)); sec->pc += 4; return 1; }
    if (strcmp(op, "ebreak") == 0) { buf_write_u32(&sec->buf, encode_i(0x001, 0, 0, 0, 0x73)); sec->pc += 4; return 1; }
    if (strcmp(op, "mret") == 0) { buf_write_u32(&sec->buf, encode_i(0x302, 0, 0, 0, 0x73)); sec->pc += 4; return 1; }
    if (strcmp(op, "sret") == 0) { buf_write_u32(&sec->buf, encode_i(0x102, 0, 0, 0, 0x73)); sec->pc += 4; return 1; }
    if (strcmp(op, "fence") == 0) { buf_write_u32(&sec->buf, encode_i(0x000, 0, 0, 0, 0x0F)); sec->pc += 4; return 1; }

    if (strcmp(op, "jal") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 1);
        buf_write_u32(&sec->buf, encode_j((int32_t)imm, rd, 0x6F)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "jalr") == 0 && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int64_t imm = resolve_imm(tokens[3], labels, abs_pc, 0);
        buf_write_u32(&sec->buf, encode_i((int32_t)imm, rs1, 0x0, rd, 0x67)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "movhi") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 0);
        buf_write_u32(&sec->buf, encode_u((int32_t)imm, rd, 0x37)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "movpc") == 0 && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 1);
        buf_write_u32(&sec->buf, encode_u((int32_t)imm, rd, 0x17)); sec->pc += 4; return 1;
    }

    // Branches
    if ((strcmp(op, "beq") == 0 || strcmp(op, "bne") == 0 || strcmp(op, "blt") == 0 || strcmp(op, "bge") == 0 || strcmp(op, "bltu") == 0 || strcmp(op, "bgeu") == 0) && count >= 4) {
        int rs1 = parse_reg(tokens[1]);
        int rs2 = parse_reg(tokens[2]);
        int64_t imm = resolve_imm(tokens[3], labels, abs_pc, 1);
        uint32_t f3 = 0;
        if (strcmp(op, "beq") == 0) f3 = 0x0;
        else if (strcmp(op, "bne") == 0) f3 = 0x1;
        else if (strcmp(op, "blt") == 0) f3 = 0x4;
        else if (strcmp(op, "bge") == 0) f3 = 0x5;
        else if (strcmp(op, "bltu") == 0) f3 = 0x6;
        else if (strcmp(op, "bgeu") == 0) f3 = 0x7;
        buf_write_u32(&sec->buf, encode_b((int32_t)imm, rs2, rs1, f3, 0x63)); sec->pc += 4; return 1;
    }

    // Loads
    if ((strcmp(op, "ldb") == 0 || strcmp(op, "ldbu") == 0 || strcmp(op, "ldh") == 0 || strcmp(op, "ldhu") == 0 || strcmp(op, "ldw") == 0 || strcmp(op, "ldwu") == 0 || strcmp(op, "ld") == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[3]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 0);
        uint32_t f3 = 0;
        if (strcmp(op, "ldb") == 0) f3 = 0x0;
        else if (strcmp(op, "ldbu") == 0) f3 = 0x4;
        else if (strcmp(op, "ldh") == 0) f3 = 0x1;
        else if (strcmp(op, "ldhu") == 0) f3 = 0x5;
        else if (strcmp(op, "ldw") == 0) f3 = 0x2;
        else if (strcmp(op, "ldwu") == 0) f3 = 0x6;
        else if (strcmp(op, "ld") == 0) f3 = 0x3;
        buf_write_u32(&sec->buf, encode_i((int32_t)imm, rs1, f3, rd, 0x03)); sec->pc += 4; return 1;
    }

    // Stores
    if ((strcmp(op, "stb") == 0 || strcmp(op, "sth") == 0 || strcmp(op, "stw") == 0 || strcmp(op, "st") == 0) && count >= 4) {
        int rs2 = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[3]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 0);
        uint32_t f3 = 0;
        if (strcmp(op, "stb") == 0) f3 = 0x0;
        else if (strcmp(op, "sth") == 0) f3 = 0x1;
        else if (strcmp(op, "stw") == 0) f3 = 0x2;
        else if (strcmp(op, "st") == 0) f3 = 0x3;
        buf_write_u32(&sec->buf, encode_s((int32_t)imm, rs2, rs1, f3, 0x23)); sec->pc += 4; return 1;
    }

    // OP-IMM
    if ((strcmp(op, "addi") == 0 || strcmp(op, "andi") == 0 || strcmp(op, "ori") == 0 || strcmp(op, "xori") == 0 || strcmp(op, "slti") == 0 || strcmp(op, "sltiu") == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int64_t imm = resolve_imm(tokens[3], labels, abs_pc, 0);
        uint32_t f3 = 0;
        if (strcmp(op, "addi") == 0) f3 = 0x0;
        else if (strcmp(op, "slti") == 0) f3 = 0x2;
        else if (strcmp(op, "sltiu") == 0) f3 = 0x3;
        else if (strcmp(op, "xori") == 0) f3 = 0x4;
        else if (strcmp(op, "ori") == 0) f3 = 0x6;
        else if (strcmp(op, "andi") == 0) f3 = 0x7;
        buf_write_u32(&sec->buf, encode_i((int32_t)imm, rs1, f3, rd, 0x13)); sec->pc += 4; return 1;
    }

    if ((strcmp(op, "slli") == 0 || strcmp(op, "srli") == 0 || strcmp(op, "srai") == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int64_t imm = resolve_imm(tokens[3], labels, abs_pc, 0);
        uint32_t f3 = (strcmp(op, "slli") == 0) ? 0x1 : 0x5;
        uint32_t f7 = (strcmp(op, "srai") == 0) ? 0x20 : 0x00;
        uint32_t insn = encode_i((int32_t)imm, rs1, f3, rd, 0x13);
        insn |= (f7 << 25);
        buf_write_u32(&sec->buf, insn); sec->pc += 4; return 1;
    }

    // OP
    if ((strcmp(op, "add") == 0 || strcmp(op, "sub") == 0 || strcmp(op, "mul") == 0 || strcmp(op, "mulh") == 0 || strcmp(op, "mulhsu") == 0 || strcmp(op, "mulhu") == 0 || strcmp(op, "div") == 0 || strcmp(op, "divu") == 0 || strcmp(op, "rem") == 0 || strcmp(op, "remu") == 0 || strcmp(op, "sll") == 0 || strcmp(op, "srl") == 0 || strcmp(op, "sra") == 0 || strcmp(op, "and") == 0 || strcmp(op, "or") == 0 || strcmp(op, "xor") == 0 || strcmp(op, "slt") == 0 || strcmp(op, "sltu") == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int rs2 = parse_reg(tokens[3]);
        uint32_t f3 = 0, f7 = 0;
        if (strcmp(op, "add") == 0) { f3 = 0x0; f7 = 0x00; }
        else if (strcmp(op, "sub") == 0) { f3 = 0x0; f7 = 0x20; }
        else if (strcmp(op, "mul") == 0) { f3 = 0x0; f7 = 0x01; }
        else if (strcmp(op, "mulh") == 0) { f3 = 0x1; f7 = 0x01; }
        else if (strcmp(op, "mulhsu") == 0) { f3 = 0x2; f7 = 0x01; }
        else if (strcmp(op, "mulhu") == 0) { f3 = 0x3; f7 = 0x01; }
        else if (strcmp(op, "div") == 0) { f3 = 0x4; f7 = 0x01; }
        else if (strcmp(op, "divu") == 0) { f3 = 0x5; f7 = 0x01; }
        else if (strcmp(op, "rem") == 0) { f3 = 0x6; f7 = 0x01; }
        else if (strcmp(op, "remu") == 0) { f3 = 0x7; f7 = 0x01; }
        else if (strcmp(op, "sll") == 0) { f3 = 0x1; f7 = 0x00; }
        else if (strcmp(op, "srl") == 0) { f3 = 0x5; f7 = 0x00; }
        else if (strcmp(op, "sra") == 0) { f3 = 0x5; f7 = 0x20; }
        else if (strcmp(op, "and") == 0) { f3 = 0x7; f7 = 0x00; }
        else if (strcmp(op, "or") == 0) { f3 = 0x6; f7 = 0x00; }
        else if (strcmp(op, "xor") == 0) { f3 = 0x4; f7 = 0x00; }
        else if (strcmp(op, "slt") == 0) { f3 = 0x2; f7 = 0x00; }
        else if (strcmp(op, "sltu") == 0) { f3 = 0x3; f7 = 0x00; }
        buf_write_u32(&sec->buf, encode_r(f7, rs2, rs1, f3, rd, 0x33)); sec->pc += 4; return 1;
    }

    // AMO
    if ((strcmp(op, "amoswap.w") == 0 || strcmp(op, "amoswap.d") == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int rs2 = parse_reg(tokens[3]);
        uint32_t f3 = (strcmp(op, "amoswap.w") == 0) ? 0x2 : 0x3;
        buf_write_u32(&sec->buf, encode_r(0x04, rs2, rs1, f3, rd, 0x2F)); sec->pc += 4; return 1;
    }

    // CSR
    if ((strncmp(op, "csrr", 4) == 0) && count >= 4) {
        int rd = parse_reg(tokens[1]);
        uint32_t csr = csr_addr_from_name(tokens[2]);
        if (csr == 0xFFFFFFFFu) {
            int64_t imm; if (!parse_int(tokens[2], &imm)) return 0; csr = (uint32_t)imm;
        }
        if (strcmp(op, "csrrw") == 0 || strcmp(op, "csrrs") == 0 || strcmp(op, "csrrc") == 0) {
            int rs1 = parse_reg(tokens[3]);
            uint32_t f3 = (strcmp(op, "csrrw") == 0) ? 0x1 : (strcmp(op, "csrrs") == 0 ? 0x2 : 0x3);
            buf_write_u32(&sec->buf, encode_i((int32_t)csr, rs1, f3, rd, 0x73)); sec->pc += 4; return 1;
        }
        if (strcmp(op, "csrrwi") == 0 || strcmp(op, "csrrsi") == 0 || strcmp(op, "csrrci") == 0) {
            int64_t zimm; if (!parse_int(tokens[3], &zimm)) return 0;
            uint32_t f3 = (strcmp(op, "csrrwi") == 0) ? 0x5 : (strcmp(op, "csrrsi") == 0 ? 0x6 : 0x7);
            buf_write_u32(&sec->buf, encode_i((int32_t)csr, (uint32_t)zimm & 0x1F, f3, rd, 0x73)); sec->pc += 4; return 1;
        }
    }

    // CAP
    if ((strcmp(op, "csetbounds") == 0 || strcmp(op, "csetperm") == 0 || strcmp(op, "cseal") == 0 || strcmp(op, "cunseal") == 0) && count >= 4) {
        int cd = parse_creg(tokens[1]);
        int cs1 = parse_creg(tokens[2]);
        int rs2 = parse_reg(tokens[3]);
        uint32_t f3 = 0x0;
        if (strcmp(op, "csetperm") == 0) f3 = 0x1;
        else if (strcmp(op, "cseal") == 0) f3 = 0x2;
        else if (strcmp(op, "cunseal") == 0) f3 = 0x3;
        buf_write_u32(&sec->buf, encode_r(0x00, rs2, cs1, f3, cd, 0x2B)); sec->pc += 4; return 1;
    }
    if ((strcmp(op, "cgettag") == 0 || strcmp(op, "cgetbase") == 0 || strcmp(op, "cgetlen") == 0) && count >= 3) {
        int rd = parse_reg(tokens[1]);
        int cs1 = parse_creg(tokens[2]);
        uint32_t f3 = (strcmp(op, "cgettag") == 0) ? 0x4 : (strcmp(op, "cgetbase") == 0 ? 0x5 : 0x6);
        buf_write_u32(&sec->buf, encode_r(0x00, 0, cs1, f3, rd, 0x2B)); sec->pc += 4; return 1;
    }
    if ((strcmp(op, "cld") == 0 || strcmp(op, "cst") == 0) && count >= 4) {
        int cd = parse_creg(tokens[1]);
        int rs1 = parse_reg(tokens[3]);
        int64_t imm = resolve_imm(tokens[2], labels, abs_pc, 0);
        uint32_t f3 = (strcmp(op, "cld") == 0) ? 0x0 : 0x1;
        buf_write_u32(&sec->buf, encode_i((int32_t)imm, rs1, f3, cd, 0x2B)); sec->pc += 4; return 1;
    }

    // TENSOR
    if ((strcmp(op, "tadd") == 0 || strcmp(op, "tmma") == 0) && count >= 4) {
        int trd = parse_treg(tokens[1]);
        int tr1 = parse_treg(tokens[2]);
        int tr2 = parse_treg(tokens[3]);
        uint32_t f7 = (strcmp(op, "tadd") == 0) ? 0x00 : 0x01;
        uint32_t f3 = (strcmp(op, "tadd") == 0) ? 0x00 : 0x01;
        buf_write_u32(&sec->buf, encode_r(f7, tr2, tr1, f3, trd, 0x5B)); sec->pc += 4; return 1;
    }
    if ((strcmp(op, "tld") == 0 || strcmp(op, "tst") == 0) && count >= 4) {
        int trd = parse_treg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        int64_t imm = resolve_imm(tokens[3], labels, abs_pc, 0);
        uint32_t f3 = (strcmp(op, "tld") == 0) ? 0x0 : 0x1;
        buf_write_u32(&sec->buf, encode_i((int32_t)imm, rs1, f3, trd, 0x5B)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "tact") == 0 && count >= 3) {
        int trd = parse_treg(tokens[1]);
        uint32_t fn = tensor_func_from_name(tokens[2]);
        buf_write_u32(&sec->buf, encode_i((int32_t)fn, trd, 0x2, trd, 0x5B)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "tcvt") == 0 && count >= 4) {
        int trd = parse_treg(tokens[1]);
        int trs = parse_treg(tokens[2]);
        uint32_t fmt = tensor_fmt_from_name(tokens[3]);
        buf_write_u32(&sec->buf, encode_i((int32_t)fmt, trs, 0x3, trd, 0x5B)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "tzero") == 0 && count >= 2) {
        int trd = parse_treg(tokens[1]);
        buf_write_u32(&sec->buf, encode_i(0, trd, 0x4, trd, 0x5B)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "tred") == 0 && count >= 4) {
        int trs = parse_treg(tokens[1]);
        int rd = parse_reg(tokens[2]);
        uint32_t opv = tensor_redop_from_name(tokens[3]);
        buf_write_u32(&sec->buf, encode_i((int32_t)opv, trs, 0x5, rd, 0x5B)); sec->pc += 4; return 1;
    }
    if (strcmp(op, "tscale") == 0 && count >= 3) {
        int trd = parse_treg(tokens[1]);
        int rs1 = parse_reg(tokens[2]);
        buf_write_u32(&sec->buf, encode_i(0, rs1, 0x6, trd, 0x5B)); sec->pc += 4; return 1;
    }

    return 0;
}

int first_pass(FILE *f, LabelTable *labels, const AsmOptions *opt) {
    char line[MAX_LINE];
    SectionKind cur = SEC_TEXT;
    uint64_t text_pc = 0, data_pc = 0, bss_pc = 0;
    bool skip_unreachable = false;
    while (fgets(line, sizeof(line), f)) {
        strip_comment(line);
        char *tokens[MAX_TOKENS];
        int count = tokenize(line, tokens, MAX_TOKENS);
        if (count == 0) continue;
        if (is_label_def(tokens[0])) {
            tokens[0][strlen(tokens[0]) - 1] = '\0';
            uint64_t base = (cur == SEC_TEXT) ? opt->text_base : (cur == SEC_DATA ? opt->data_base : opt->bss_base);
            uint64_t pc = (cur == SEC_TEXT) ? text_pc : (cur == SEC_DATA ? data_pc : bss_pc);
            label_add(labels, tokens[0], base + pc);
            for (int i = 1; i < count; i++) tokens[i - 1] = tokens[i];
            count--;
            if (count == 0) { skip_unreachable = false; continue; }
            skip_unreachable = false;
        }

        SectionKind next;
        if (is_section_directive(tokens, count, &next)) {
            cur = next;
            if (cur != SEC_TEXT) skip_unreachable = false;
            continue;
        }

        if (opt->optimize && cur == SEC_TEXT && skip_unreachable && tokens[0][0] != '.') {
            continue;
        }

        if (tokens[0][0] == '.') {
            uint64_t *pc = (cur == SEC_TEXT) ? &text_pc : (cur == SEC_DATA ? &data_pc : &bss_pc);
            uint64_t base = (cur == SEC_TEXT) ? opt->text_base : (cur == SEC_DATA ? opt->data_base : opt->bss_base);

            if (strcmp(tokens[0], ".org") == 0 && count >= 2) {
                int64_t v; if (!parse_int(tokens[1], &v)) return 0;
                if ((uint64_t)v < base) return 0;
                *pc = (uint64_t)v - base;
            } else if (strcmp(tokens[0], ".align") == 0 && count >= 2) {
                int64_t v; if (!parse_int(tokens[1], &v)) return 0;
                if (v < 0) return 0;
                uint64_t align = (v >= 63) ? (1ull << 63) : (1ull << v);
                *pc = align_up(*pc, align);
            } else if (strcmp(tokens[0], ".byte") == 0) *pc += (uint64_t)(count - 1);
            else if (strcmp(tokens[0], ".half") == 0) *pc += (uint64_t)(2 * (count - 1));
            else if (strcmp(tokens[0], ".word") == 0) *pc += (uint64_t)(4 * (count - 1));
            else if (strcmp(tokens[0], ".dword") == 0) *pc += (uint64_t)(8 * (count - 1));
            else if (strcmp(tokens[0], ".ascii") == 0 || strcmp(tokens[0], ".asciz") == 0) {
                if (cur == SEC_BSS) return 0;
                if (count >= 2 && tokens[1][0] == '"') {
                    size_t len = strlen(tokens[1]);
                    if (len > 0 && tokens[1][len - 1] == '"') len--;
                    *pc += (uint64_t)(len - 1);
                    if (strcmp(tokens[0], ".asciz") == 0) *pc += 1;
                }
            }
        } else {
            if (cur != SEC_TEXT) return 0;
            if (strcmp(tokens[0], "li") == 0 && count >= 3) {
                int64_t imm = 0;
                if (parse_int(tokens[2], &imm)) {
                    text_pc += (imm >= -2048 && imm <= 2047) ? 4 : 8;
                } else {
                    text_pc += 8;
                }
            } else {
                text_pc += 4;
            }
            if (opt->optimize && is_unconditional_jump(tokens, count)) {
                skip_unreachable = true;
            }
        }
    }
    return 1;
}

int assemble_file(const char *in_path, const char *out_path, bool elf_output, const AsmOptions *opt) {
    FILE *f = fopen(in_path, "r");
    if (!f) return 0;
    LabelTable labels = {0};
    if (!first_pass(f, &labels, opt)) { fclose(f); return 0; }
    rewind(f);

    Section text = {0}, data = {0}, bss = {0};
    buf_init(&text.buf);
    buf_init(&data.buf);
    text.base = opt->text_base;
    data.base = opt->data_base;
    bss.base = opt->bss_base;
    text.pc = data.pc = bss.pc = 0;

    SectionKind cur = SEC_TEXT;
    char line[MAX_LINE];
    int ok = 1;
    int line_no = 0;
    bool skip_unreachable = false;
    while (fgets(line, sizeof(line), f)) {
        line_no++;
        strip_comment(line);
        char *tokens[MAX_TOKENS];
        int count = tokenize(line, tokens, MAX_TOKENS);
        if (count == 0) continue;
        if (is_label_def(tokens[0])) {
            tokens[0][strlen(tokens[0]) - 1] = '\0';
            for (int i = 1; i < count; i++) tokens[i - 1] = tokens[i];
            count--;
            if (count == 0) { skip_unreachable = false; continue; }
            skip_unreachable = false;
        }

        SectionKind next;
        if (is_section_directive(tokens, count, &next)) {
            cur = next;
            if (cur != SEC_TEXT) skip_unreachable = false;
            continue;
        }

        if (opt->optimize && cur == SEC_TEXT && skip_unreachable && tokens[0][0] != '.') {
            continue;
        }

        Section *sec = (cur == SEC_TEXT) ? &text : (cur == SEC_DATA ? &data : &bss);
        if (!assemble_line(sec, cur, tokens, count, &labels)) {
            uint64_t abs_pc = sec->base + sec->pc;
            fprintf(stderr, "assemble error at line %d pc=0x%llx\n", line_no, (unsigned long long)abs_pc);
            ok = 0;
            break;
        }
        if (opt->optimize && cur == SEC_TEXT && is_unconditional_jump(tokens, count)) {
            skip_unreachable = true;
        }
    }
    fclose(f);

    if (!ok) { free(text.buf.data); free(data.buf.data); return 0; }

    if (opt->optimize) {
        optimize_text_section(&text);
    }

    if (!elf_output && (data.buf.size > 0 || bss.pc > 0 || opt->data_base != opt->text_base || opt->bss_base != opt->text_base)) {
        free(text.buf.data); free(data.buf.data); return 0;
    }

    uint64_t entry = opt->text_base;
    uint64_t start_addr = 0;
    if (label_find(&labels, "start", &start_addr)) entry = start_addr;

    int ok_write = 1;
    if (elf_output) {
        ok_write = write_elf_file_sections(out_path, &text, &data, &bss, entry, opt->seg_align);
    } else {
        FILE *out = fopen(out_path, "wb");
        if (!out) ok_write = 0;
        else {
            if (fwrite(text.buf.data, 1, text.buf.size, out) != text.buf.size) ok_write = 0;
            fclose(out);
        }
    }
    free(text.buf.data);
    free(data.buf.data);
    return ok_write;
}
