#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t fp_encode(float v, int ebits, int mbits, int bias) {
    if (isnan(v)) {
        return (uint8_t)(((1u << ebits) - 1u) << mbits) | (1u << (mbits - 1));
    }
    if (isinf(v)) {
        uint8_t sign = signbit(v) ? 1u : 0u;
        return (uint8_t)((sign << (ebits + mbits)) | (((1u << ebits) - 1u) << mbits));
    }

    uint8_t sign = signbit(v) ? 1u : 0u;
    float av = fabsf(v);
    if (av == 0.0f) return (uint8_t)(sign << (ebits + mbits));

    int exp2;
    float frac = frexpf(av, &exp2);
    exp2 -= 1;
    int exp = exp2 + bias;

    if (exp <= 0) {
        return (uint8_t)(sign << (ebits + mbits));
    }
    if (exp >= (1 << ebits) - 1) {
        return (uint8_t)((sign << (ebits + mbits)) | (((1u << ebits) - 1u) << mbits));
    }

    float mant_f = (frac * 2.0f - 1.0f) * (float)(1 << mbits);
    uint32_t mant = (uint32_t)lrintf(mant_f);
    if (mant == (1u << mbits)) {
        mant = 0;
        exp += 1;
        if (exp >= (1 << ebits) - 1) {
            return (uint8_t)((sign << (ebits + mbits)) | (((1u << ebits) - 1u) << mbits));
        }
    }

    return (uint8_t)((sign << (ebits + mbits)) | ((uint32_t)exp << mbits) | (mant & ((1u << mbits) - 1u)));
}

static uint8_t fp8_e4m3_from_f32(float v) { return fp_encode(v, 4, 3, 7); }
static uint8_t fp4_e2m1_from_f32(float v) { return fp_encode(v, 2, 1, 1); }

static void emit32(uint8_t *buf, size_t *off, uint32_t insn) {
    buf[*off + 0] = (uint8_t)(insn & 0xFF);
    buf[*off + 1] = (uint8_t)((insn >> 8) & 0xFF);
    buf[*off + 2] = (uint8_t)((insn >> 16) & 0xFF);
    buf[*off + 3] = (uint8_t)((insn >> 24) & 0xFF);
    *off += 4;
}

static uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    return (uimm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_u(int32_t imm20, uint32_t rd, uint32_t opcode) {
    return ((uint32_t)imm20 << 12) | (rd << 7) | opcode;
}

static void emit_mov_imm(uint8_t *buf, size_t *off, uint32_t rd, uint32_t imm) {
    uint32_t hi = (imm + 0x800) >> 12;
    uint32_t lo = imm & 0xFFF;
    emit32(buf, off, encode_u((int32_t)hi, rd, 0x37));
    emit32(buf, off, encode_i((int32_t)lo, rd, 0x0, rd, 0x13));
}

int main(void) {
    const size_t mem_size = 0x4000;
    uint8_t *buf = (uint8_t *)calloc(1, mem_size);
    if (!buf) return 1;

    size_t off = 0;

    // r1 = 0x1000 (A fp8)
    // r2 = 0x1100 (B fp8)
    // r3 = 0x2000 (C fp32 out)
    // r4 = 0x3000 (fp4 in)
    // r5 = 0x3080 (fp4 out)
    emit_mov_imm(buf, &off, 1, 0x1000);
    emit_mov_imm(buf, &off, 2, 0x1100);
    emit_mov_imm(buf, &off, 3, 0x2000);
    emit_mov_imm(buf, &off, 4, 0x3000);
    emit_mov_imm(buf, &off, 5, 0x3080);

    // tzero tr0; tcvt tr0,tr0,FP8(E4M3); tld tr0,(r1),16
    emit32(buf, &off, encode_i(0, 0, 0x4, 0, 0x5B));
    emit32(buf, &off, encode_i(3, 0, 0x3, 0, 0x5B));
    emit32(buf, &off, encode_i(16, 1, 0x0, 0, 0x5B));

    // tzero tr1; tcvt tr1,tr1,FP8(E4M3); tld tr1,(r2),16
    emit32(buf, &off, encode_i(0, 1, 0x4, 1, 0x5B));
    emit32(buf, &off, encode_i(3, 1, 0x3, 1, 0x5B));
    emit32(buf, &off, encode_i(16, 2, 0x0, 1, 0x5B));

    // tzero tr2; tmma tr2,tr0,tr1; tst tr2,(r3),64
    emit32(buf, &off, encode_i(0, 2, 0x4, 2, 0x5B));
    emit32(buf, &off, encode_r(0x01, 1, 0, 0x1, 2, 0x5B));
    emit32(buf, &off, encode_i(64, 3, 0x1, 2, 0x5B));

    // tzero tr3; tcvt tr3,tr3,FP4(E2M1); tld tr3,(r4),8; tst tr3,(r5),8
    emit32(buf, &off, encode_i(0, 3, 0x4, 3, 0x5B));
    emit32(buf, &off, encode_i(6, 3, 0x3, 3, 0x5B));
    emit32(buf, &off, encode_i(8, 4, 0x0, 3, 0x5B));
    emit32(buf, &off, encode_i(8, 5, 0x1, 3, 0x5B));

    // ebreak
    emit32(buf, &off, encode_i(1, 0, 0x0, 0, 0x73));

    // Fill FP8 A/B tiles with 1.0f
    for (int i = 0; i < 256; i++) {
        buf[0x1000 + i] = fp8_e4m3_from_f32(1.0f);
        buf[0x1100 + i] = fp8_e4m3_from_f32(1.0f);
    }

    // Fill FP4 tile with a simple ramp (0..7)
    for (int i = 0; i < 256; i++) {
        float v = (float)(i & 7);
        uint8_t enc = fp4_e2m1_from_f32(v) & 0xFu;
        size_t byte = 0x3000 + (i / 2);
        if (i & 1) buf[byte] = (uint8_t)((buf[byte] & 0x0Fu) | (enc << 4));
        else buf[byte] = (uint8_t)((buf[byte] & 0xF0u) | enc);
    }

    FILE *f = fopen("tensor_test.bin", "wb");
    if (!f) { free(buf); return 1; }
    fwrite(buf, 1, mem_size, f);
    fclose(f);
    free(buf);
    return 0;
}
