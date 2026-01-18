#include "cpu.h"
#include "isa.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define UART_TX_ADDR 0x10000000ull
#define UART_RX_ADDR 0x10000004ull
#define UART_STATUS_ADDR 0x10000008ull
#define UART_RX_SIZE 256u

#define SYS_WRITE 1u
#define SYS_READ 2u
#define SYS_EXIT 3u

static bool cap_check(CapReg c, uint64_t addr, uint64_t len, uint16_t need, uint64_t *subcode);
static void trap_entry(Cpu *c, uint64_t cause, uint64_t tval, bool is_interrupt);

static inline uint32_t rd_field(uint32_t insn) { return get_bits(insn, 11, 7); }
static inline uint32_t rs1_field(uint32_t insn) { return get_bits(insn, 19, 15); }
static inline uint32_t rs2_field(uint32_t insn) { return get_bits(insn, 24, 20); }
static inline uint32_t funct3(uint32_t insn) { return get_bits(insn, 14, 12); }
static inline uint32_t funct7(uint32_t insn) { return get_bits(insn, 31, 25); }

static inline int64_t imm_i(uint32_t insn) {
    return sign_extend(get_bits(insn, 31, 20), 12);
}

static inline int64_t imm_s(uint32_t insn) {
    uint32_t hi = get_bits(insn, 31, 25);
    uint32_t lo = get_bits(insn, 11, 7);
    return sign_extend((hi << 5) | lo, 12);
}

static inline int64_t imm_b(uint32_t insn) {
    uint32_t bit12 = get_bits(insn, 31, 31);
    uint32_t bit11 = get_bits(insn, 7, 7);
    uint32_t bits10_5 = get_bits(insn, 30, 25);
    uint32_t bits4_1 = get_bits(insn, 11, 8);
    uint32_t imm = (bit12 << 12) | (bit11 << 11) | (bits10_5 << 5) | (bits4_1 << 1);
    return sign_extend(imm, 13);
}

static inline int64_t imm_u(uint32_t insn) {
    return sign_extend(get_bits(insn, 31, 12), 20) << 12;
}

static inline int64_t imm_j(uint32_t insn) {
    uint32_t bit20 = get_bits(insn, 31, 31);
    uint32_t bits10_1 = get_bits(insn, 30, 21);
    uint32_t bit11 = get_bits(insn, 20, 20);
    uint32_t bits19_12 = get_bits(insn, 19, 12);
    uint32_t imm = (bit20 << 20) | (bits19_12 << 12) | (bit11 << 11) | (bits10_1 << 1);
    return sign_extend(imm, 21);
}

#define MSTATUS_MIE   (1ull << 0)
#define MSTATUS_SIE   (1ull << 1)
#define MSTATUS_MPIE  (1ull << 4)
#define MSTATUS_SPIE  (1ull << 5)
#define MSTATUS_CAP   (1ull << 8)
#define MSTATUS_TS_MASK (3ull << 9)
#define MSTATUS_MPP_SHIFT 11
#define MSTATUS_MPP_MASK (3ull << MSTATUS_MPP_SHIFT)
#define MSTATUS_SPP  (1ull << 13)

#define SSTATUS_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP | MSTATUS_TS_MASK | MSTATUS_CAP)

enum {
    CSR_SSTATUS = 0x100,
    CSR_SIE = 0x104,
    CSR_STVEC = 0x105,
    CSR_SSCRATCH = 0x140,
    CSR_SEPC = 0x141,
    CSR_SCAUSE = 0x142,
    CSR_STVAL = 0x143,
    CSR_SIP = 0x144,

    CSR_MSTATUS = 0x300,
    CSR_MEDELEG = 0x302,
    CSR_MIDELEG = 0x303,
    CSR_MIE = 0x304,
    CSR_MTVEC = 0x305,
    CSR_MSCRATCH = 0x340,
    CSR_MEPC = 0x341,
    CSR_MCAUSE = 0x342,
    CSR_MTVAL = 0x343,
    CSR_MIP = 0x344,

    CSR_CYCLE = 0xC00,
    CSR_TIME = 0xC01,
    CSR_INSTRET = 0xC02,
};

static inline void write_reg(Cpu *c, uint32_t rd, uint64_t val) {
    if (rd == 0) return;
    c->regs[rd] = val;
}

static inline void uart_write(uint64_t val, size_t size) {
    for (size_t i = 0; i < size; i++) {
        uint8_t ch = (uint8_t)((val >> (8 * i)) & 0xFF);
        fputc((int)ch, stdout);
    }
    fflush(stdout);
}

static void uart_rx_fill(Cpu *c) {
    if (c->uart_rx_count >= UART_RX_SIZE) return;
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int r = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    if (r <= 0 || !FD_ISSET(STDIN_FILENO, &rfds)) return;

    uint8_t tmp[64];
    ssize_t n = read(STDIN_FILENO, tmp, sizeof(tmp));
    if (n <= 0) return;
    for (ssize_t i = 0; i < n; i++) {
        if (c->uart_rx_count >= UART_RX_SIZE) break;
        c->uart_rx[c->uart_rx_head] = tmp[i];
        c->uart_rx_head = (c->uart_rx_head + 1) % UART_RX_SIZE;
        c->uart_rx_count++;
    }
}

static bool uart_rx_pop(Cpu *c, uint8_t *out) {
    uart_rx_fill(c);
    if (c->uart_rx_count == 0) return false;
    *out = c->uart_rx[c->uart_rx_tail];
    c->uart_rx_tail = (c->uart_rx_tail + 1) % UART_RX_SIZE;
    c->uart_rx_count--;
    return true;
}

static uint8_t uart_rx_status(Cpu *c) {
    uart_rx_fill(c);
    return (c->uart_rx_count > 0) ? 1 : 0;
}

static Trap syscall_handle(Cpu *c, Mem *m) {
    uint64_t a0 = c->regs[10];
    uint64_t a1 = c->regs[11];
    uint64_t a2 = c->regs[12];
    uint64_t a7 = c->regs[17];

    if (a7 == SYS_EXIT) {
        return TRAP_EBREAK;
    }

    if (a7 == SYS_WRITE) {
        if (a0 != 1 && a0 != 2) { c->regs[10] = 0; return TRAP_NONE; }
        if (a2 == 0) { c->regs[10] = 0; return TRAP_NONE; }
        if (a2 > 4096) a2 = 4096;
        if ((c->mstatus & MSTATUS_CAP) && c->mode != MODE_M) {
            uint64_t sub = 0;
            if (!cap_check(c->caps[0], a1, a2, 0x1, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
        }
        uint8_t buf[4096];
        if (!mem_read(m, a1, buf, (size_t)a2)) { trap_entry(c, 5, a1, false); return TRAP_NONE; }
        FILE *out = (a0 == 2) ? stderr : stdout;
        size_t n = fwrite(buf, 1, (size_t)a2, out);
        fflush(out);
        c->regs[10] = (uint64_t)n;
        return TRAP_NONE;
    }

    if (a7 == SYS_READ) {
        if (a0 != 0) { c->regs[10] = 0; return TRAP_NONE; }
        if (a2 == 0) { c->regs[10] = 0; return TRAP_NONE; }
        if (a2 > 4096) a2 = 4096;
        if ((c->mstatus & MSTATUS_CAP) && c->mode != MODE_M) {
            uint64_t sub = 0;
            if (!cap_check(c->caps[0], a1, a2, 0x2, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
        }
        uint8_t buf[4096];
        ssize_t n = read(STDIN_FILENO, buf, (size_t)a2);
        if (n < 0) n = 0;
        if (n > 0) {
            if (!mem_write(m, a1, buf, (size_t)n)) { trap_entry(c, 7, a1, false); return TRAP_NONE; }
        }
        c->regs[10] = (uint64_t)n;
        return TRAP_NONE;
    }

    return TRAP_UNIMPLEMENTED;
}

static inline float bits_to_f32(uint32_t v) {
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

static inline uint32_t f32_to_bits(float f) {
    uint32_t v;
    memcpy(&v, &f, sizeof(v));
    return v;
}

static uint16_t f32_to_bf16(float f) {
    uint32_t u = f32_to_bits(f);
    uint32_t lsb = (u >> 16) & 1u;
    uint32_t rounding = 0x7FFFu + lsb;
    return (uint16_t)((u + rounding) >> 16);
}

static float bf16_to_f32(uint16_t v) {
    uint32_t u = ((uint32_t)v) << 16;
    return bits_to_f32(u);
}

static uint16_t f32_to_f16(float f) {
    uint32_t x = f32_to_bits(f);
    uint32_t sign = (x >> 31) & 1u;
    int32_t exp = (int32_t)((x >> 23) & 0xFF) - 127 + 15;
    uint32_t mant = x & 0x7FFFFFu;
    if (exp <= 0) {
        if (exp < -10) return (uint16_t)(sign << 15);
        mant |= 0x800000u;
        uint32_t shift = (uint32_t)(14 - exp);
        uint32_t half = (mant >> shift) + ((mant >> (shift - 1)) & 1u);
        return (uint16_t)((sign << 15) | (half & 0x3FFu));
    }
    if (exp >= 31) {
        return (uint16_t)((sign << 15) | 0x7C00u);
    }
    uint32_t half = (mant >> 13);
    if (mant & 0x1000u) half++;
    return (uint16_t)((sign << 15) | ((uint32_t)exp << 10) | (half & 0x3FFu));
}

static float f16_to_f32(uint16_t h) {
    uint32_t sign = (h >> 15) & 1u;
    uint32_t exp = (h >> 10) & 0x1Fu;
    uint32_t mant = h & 0x3FFu;
    uint32_t out;
    if (exp == 0) {
        if (mant == 0) {
            out = sign << 31;
        } else {
            exp = 127 - 15 + 1;
            while ((mant & 0x400u) == 0) { mant <<= 1; exp--; }
            mant &= 0x3FFu;
            out = (sign << 31) | (exp << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        out = (sign << 31) | 0x7F800000u | (mant << 13);
    } else {
        exp = exp - 15 + 127;
        out = (sign << 31) | (exp << 23) | (mant << 13);
    }
    return bits_to_f32(out);
}

static inline int8_t sat_int8(int32_t v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}

static inline bool is_float_fmt(TensorFmt f) {
    return f == TFMT_FP32 || f == TFMT_FP16 || f == TFMT_BF16 || f == TFMT_FP8_E4M3 || f == TFMT_FP8_E5M2 || f == TFMT_FP4_E2M1;
}

static inline int fmt_bytes(TensorFmt f) {
    switch (f) {
        case TFMT_FP32: return 4;
        case TFMT_FP16: return 2;
        case TFMT_BF16: return 2;
        case TFMT_INT8: return 1;
        case TFMT_FP8_E4M3: return 1;
        case TFMT_FP8_E5M2: return 1;
        case TFMT_FP4_E2M1: return 1;
        default: return 0;
    }
}

static inline bool fmt_supported(TensorFmt f) {
    return f == TFMT_FP32 || f == TFMT_FP16 || f == TFMT_BF16 || f == TFMT_FP8_E4M3 || f == TFMT_FP8_E5M2 || f == TFMT_INT8 || f == TFMT_FP4_E2M1;
}

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
        // flush-to-zero for FP8/FP4
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

static float fp_decode(uint8_t b, int ebits, int mbits, int bias) {
    uint32_t sign = (b >> (ebits + mbits)) & 0x1;
    uint32_t exp = (b >> mbits) & ((1u << ebits) - 1u);
    uint32_t mant = b & ((1u << mbits) - 1u);
    if (exp == 0) {
        if (mant == 0) return sign ? -0.0f : 0.0f;
        // subnormals flush-to-zero for FP8/FP4
        return sign ? -0.0f : 0.0f;
    }
    if (exp == (1u << ebits) - 1u) {
        return mant ? NAN : (sign ? -INFINITY : INFINITY);
    }
    float frac = 1.0f + ((float)mant / (float)(1u << mbits));
    int e = (int)exp - bias;
    float v = ldexpf(frac, e);
    return sign ? -v : v;
}

static uint8_t fp8_e4m3_from_f32(float v) { return fp_encode(v, 4, 3, 7); }
static uint8_t fp8_e5m2_from_f32(float v) { return fp_encode(v, 5, 2, 15); }
static float f32_from_fp8_e4m3(uint8_t v) { return fp_decode(v, 4, 3, 7); }
static float f32_from_fp8_e5m2(uint8_t v) { return fp_decode(v, 5, 2, 15); }

static uint8_t fp4_e2m1_from_f32(float v) { return fp_encode(v, 2, 1, 1); }
static float f32_from_fp4_e2m1(uint8_t v) { return fp_decode(v, 2, 1, 1); }

static float quantize_to_fmt(TensorFmt f, float v) {
    switch (f) {
        case TFMT_FP32: return v;
        case TFMT_FP16: return f16_to_f32(f32_to_f16(v));
        case TFMT_BF16: return bf16_to_f32(f32_to_bf16(v));
        case TFMT_FP8_E4M3: return f32_from_fp8_e4m3(fp8_e4m3_from_f32(v));
        case TFMT_FP8_E5M2: return f32_from_fp8_e5m2(fp8_e5m2_from_f32(v));
        case TFMT_FP4_E2M1: return f32_from_fp4_e2m1(fp4_e2m1_from_f32(v));
        case TFMT_INT8: return (float)sat_int8((int32_t)lrintf(v));
        default: return v;
    }
}

static inline uint16_t cap_perm(CapReg c) { return c.perm; }

static bool cap_check(CapReg c, uint64_t addr, uint64_t len, uint16_t need, uint64_t *subcode) {
    if (!c.tag) { *subcode = 0x3; return false; }
    if (c.sealed) { *subcode = 0x4; return false; }
    if (addr < c.base || addr + len < addr || (addr + len) > (uint64_t)c.base + c.len) {
        *subcode = 0x1; return false;
    }
    if ((cap_perm(c) & need) != need) { *subcode = 0x2; return false; }
    return true;
}

static bool csr_read(Cpu *c, uint32_t csr, uint64_t *out) {
    switch (csr) {
        case CSR_MSTATUS: *out = c->mstatus; return true;
        case CSR_MIE: *out = c->mie; return true;
        case CSR_MEDELEG: *out = c->medeleg; return true;
        case CSR_MIDELEG: *out = c->mideleg; return true;
        case CSR_MTVEC: *out = c->mtvec; return true;
        case CSR_MIP: *out = c->mip; return true;
        case CSR_MSCRATCH: *out = c->mscratch; return true;
        case CSR_MEPC: *out = c->mepc; return true;
        case CSR_MCAUSE: *out = c->mcause; return true;
        case CSR_MTVAL: *out = c->mtval; return true;
        case CSR_SSTATUS: *out = c->mstatus & SSTATUS_MASK; return true;
        case CSR_SIE: *out = c->sie; return true;
        case CSR_STVEC: *out = c->stvec; return true;
        case CSR_SIP: *out = c->sip; return true;
        case CSR_SSCRATCH: *out = c->sscratch; return true;
        case CSR_SEPC: *out = c->sepc; return true;
        case CSR_SCAUSE: *out = c->scause; return true;
        case CSR_STVAL: *out = c->stval; return true;
        case CSR_CYCLE: *out = c->cycle; return true;
        case CSR_TIME: *out = c->time; return true;
        case CSR_INSTRET: *out = c->instret; return true;
        default: return false;
    }
}

static bool csr_write(Cpu *c, uint32_t csr, uint64_t val) {
    switch (csr) {
        case CSR_MSTATUS: c->mstatus = val; return true;
        case CSR_MIE: c->mie = val; return true;
        case CSR_MEDELEG: c->medeleg = val; return true;
        case CSR_MIDELEG: c->mideleg = val; return true;
        case CSR_MTVEC: c->mtvec = val & ~0x3ull; return true;
        case CSR_MIP: c->mip = val; return true;
        case CSR_MSCRATCH: c->mscratch = val; return true;
        case CSR_MEPC: c->mepc = val; return true;
        case CSR_MCAUSE: c->mcause = val; return true;
        case CSR_MTVAL: c->mtval = val; return true;
        case CSR_SSTATUS:
            c->mstatus = (c->mstatus & ~SSTATUS_MASK) | (val & SSTATUS_MASK);
            return true;
        case CSR_SIE: c->sie = val; return true;
        case CSR_STVEC: c->stvec = val & ~0x3ull; return true;
        case CSR_SIP: c->sip = val; return true;
        case CSR_SSCRATCH: c->sscratch = val; return true;
        case CSR_SEPC: c->sepc = val; return true;
        case CSR_SCAUSE: c->scause = val; return true;
        case CSR_STVAL: c->stval = val; return true;
        default: return false;
    }
}

static bool csr_access_ok(Cpu *c, uint32_t csr) {
    uint32_t level = (csr >> 8) & 0x3;
    if (level == 0x3) return c->mode == MODE_M;
    if (level == 0x1) return c->mode == MODE_M || c->mode == MODE_S;
    return true;
}

static void trap_entry(Cpu *c, uint64_t cause, uint64_t tval, bool is_interrupt) {
    bool to_s = false;
    if (c->mode != MODE_M) {
        uint64_t deleg = is_interrupt ? c->mideleg : c->medeleg;
        if (cause < 64 && (deleg & (1ull << cause))) to_s = true;
    }
    if (to_s) {
        c->sepc = c->pc;
        c->scause = cause | (is_interrupt ? (1ull << 63) : 0);
        c->stval = tval;
        uint64_t prev = (c->mstatus & MSTATUS_SIE) ? 1 : 0;
        c->mstatus = (c->mstatus & ~MSTATUS_SPIE) | (prev ? MSTATUS_SPIE : 0);
        c->mstatus &= ~MSTATUS_SIE;
        if (c->mode == MODE_S) c->mstatus |= MSTATUS_SPP; else c->mstatus &= ~MSTATUS_SPP;
        c->mode = MODE_S;
        c->pc = c->stvec;
    } else {
        c->mepc = c->pc;
        c->mcause = cause | (is_interrupt ? (1ull << 63) : 0);
        c->mtval = tval;
        uint64_t prev = (c->mstatus & MSTATUS_MIE) ? 1 : 0;
        c->mstatus = (c->mstatus & ~MSTATUS_MPIE) | (prev ? MSTATUS_MPIE : 0);
        c->mstatus &= ~MSTATUS_MIE;
        c->mstatus = (c->mstatus & ~MSTATUS_MPP_MASK) | ((uint64_t)c->mode << MSTATUS_MPP_SHIFT);
        c->mode = MODE_M;
        c->pc = c->mtvec;
    }
}

void cpu_init(Cpu *c, uint64_t entry) {
    memset(c, 0, sizeof(*c));
    c->pc = entry;
    c->mode = MODE_M;
    c->mstatus = MSTATUS_CAP;
    for (int i = 0; i < 32; i++) {
        c->caps[i].base = 0;
        c->caps[i].len = 0xFFFFFFFFu;
        c->caps[i].perm = 0xFFFFu;
        c->caps[i].otype = 0;
        c->caps[i].tag = (i == 0 || i == 31);
        c->caps[i].sealed = false;
    }
    for (int t = 0; t < 8; t++) c->tregs[t].fmt = TFMT_FP32;
}

void cpu_dump_regs(const Cpu *c) {
    for (int i = 0; i < 32; i++) {
        printf("r%-2d = 0x%016llx%s", i, (unsigned long long)c->regs[i], (i % 4 == 3) ? "\n" : "  ");
    }
}

static Trap tensor_tld(Cpu *c, Mem *m, uint32_t trd, uint64_t base, int64_t stride) {
    TensorReg *d = &c->tregs[trd];
    if (!fmt_supported(d->fmt)) return TRAP_UNIMPLEMENTED;
    int elem = fmt_bytes(d->fmt);
    if ((base & (elem - 1)) != 0) return TRAP_LOAD_MISALIGNED;
    for (int y = 0; y < 16; y++) {
        uint64_t row = base + (uint64_t)(y * stride);
        for (int x = 0; x < 16; x++) {
            uint64_t addr = row + (uint64_t)(x * elem);
            float v = 0.0f;
            if (d->fmt == TFMT_FP32) {
                uint32_t u; if (!mem_read_u32(m, addr, &u)) return TRAP_LOAD_FAULT; v = bits_to_f32(u);
            } else if (d->fmt == TFMT_FP16) {
                uint16_t u; if (!mem_read_u16(m, addr, &u)) return TRAP_LOAD_FAULT; v = f16_to_f32(u);
            } else if (d->fmt == TFMT_BF16) {
                uint16_t u; if (!mem_read_u16(m, addr, &u)) return TRAP_LOAD_FAULT; v = bf16_to_f32(u);
            } else if (d->fmt == TFMT_FP8_E4M3) {
                uint8_t u; if (!mem_read_u8(m, addr, &u)) return TRAP_LOAD_FAULT; v = f32_from_fp8_e4m3(u);
            } else if (d->fmt == TFMT_FP8_E5M2) {
                uint8_t u; if (!mem_read_u8(m, addr, &u)) return TRAP_LOAD_FAULT; v = f32_from_fp8_e5m2(u);
            } else if (d->fmt == TFMT_FP4_E2M1) {
                uint8_t b; if (!mem_read_u8(m, row + (uint64_t)(x / 2), &b)) return TRAP_LOAD_FAULT;
                uint8_t nib = (x & 1) ? (b >> 4) : (b & 0xF);
                v = f32_from_fp4_e2m1(nib & 0xFu);
            } else if (d->fmt == TFMT_INT8) {
                uint8_t u; if (!mem_read_u8(m, addr, &u)) return TRAP_LOAD_FAULT; v = (int8_t)u;
            }
            d->v[y * 16 + x] = v;
        }
    }
    return TRAP_NONE;
}

static Trap tensor_tst(Cpu *c, Mem *m, uint32_t trs, uint64_t base, int64_t stride) {
    TensorReg *s = &c->tregs[trs];
    if (!fmt_supported(s->fmt)) return TRAP_UNIMPLEMENTED;
    int elem = fmt_bytes(s->fmt);
    if ((base & (elem - 1)) != 0) return TRAP_STORE_MISALIGNED;
    for (int y = 0; y < 16; y++) {
        uint64_t row = base + (uint64_t)(y * stride);
        for (int x = 0; x < 16; x++) {
            uint64_t addr = row + (uint64_t)(x * elem);
            float v = s->v[y * 16 + x];
            if (s->fmt == TFMT_FP32) {
                uint32_t u = f32_to_bits(v); if (!mem_write_u32(m, addr, u)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_FP16) {
                uint16_t u = f32_to_f16(v); if (!mem_write_u16(m, addr, u)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_BF16) {
                uint16_t u = f32_to_bf16(v); if (!mem_write_u16(m, addr, u)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_FP8_E4M3) {
                uint8_t u = fp8_e4m3_from_f32(v); if (!mem_write_u8(m, addr, u)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_FP8_E5M2) {
                uint8_t u = fp8_e5m2_from_f32(v); if (!mem_write_u8(m, addr, u)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_FP4_E2M1) {
                uint8_t u = fp4_e2m1_from_f32(v) & 0xFu;
                uint64_t baddr = row + (uint64_t)(x / 2);
                uint8_t b; if (!mem_read_u8(m, baddr, &b)) return TRAP_STORE_FAULT;
                if (x & 1) b = (uint8_t)((b & 0x0Fu) | (u << 4));
                else b = (uint8_t)((b & 0xF0u) | u);
                if (!mem_write_u8(m, baddr, b)) return TRAP_STORE_FAULT;
            } else if (s->fmt == TFMT_INT8) {
                int8_t u = sat_int8((int32_t)lrintf(v)); if (!mem_write_u8(m, addr, (uint8_t)u)) return TRAP_STORE_FAULT;
            }
        }
    }
    return TRAP_NONE;
}

static Trap tensor_tadd(Cpu *c, uint32_t d, uint32_t a, uint32_t b) {
    TensorReg *td = &c->tregs[d];
    TensorReg *ta = &c->tregs[a];
    TensorReg *tb = &c->tregs[b];
    if (is_float_fmt(td->fmt) != is_float_fmt(ta->fmt) || is_float_fmt(td->fmt) != is_float_fmt(tb->fmt)) return TRAP_ILLEGAL_INSN;
    if (!fmt_supported(td->fmt) || !fmt_supported(ta->fmt) || !fmt_supported(tb->fmt)) return TRAP_UNIMPLEMENTED;
    for (int i = 0; i < 256; i++) {
        float v = ta->v[i] + tb->v[i];
        td->v[i] = quantize_to_fmt(td->fmt, v);
    }
    return TRAP_NONE;
}

static Trap tensor_tmma(Cpu *c, uint32_t d, uint32_t a, uint32_t b) {
    TensorReg *td = &c->tregs[d];
    TensorReg *ta = &c->tregs[a];
    TensorReg *tb = &c->tregs[b];
    if (is_float_fmt(td->fmt) != is_float_fmt(ta->fmt) || is_float_fmt(td->fmt) != is_float_fmt(tb->fmt)) return TRAP_ILLEGAL_INSN;
    if (!fmt_supported(td->fmt) || !fmt_supported(ta->fmt) || !fmt_supported(tb->fmt)) return TRAP_UNIMPLEMENTED;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            float acc = td->v[i * 16 + j];
            for (int k = 0; k < 16; k++) {
                acc += ta->v[i * 16 + k] * tb->v[k * 16 + j];
            }
            td->v[i * 16 + j] = quantize_to_fmt(td->fmt, acc);
        }
    }
    return TRAP_NONE;
}

static Trap tensor_tact(Cpu *c, uint32_t d, uint32_t func) {
    TensorReg *td = &c->tregs[d];
    if (!fmt_supported(td->fmt)) return TRAP_UNIMPLEMENTED;
    for (int i = 0; i < 256; i++) {
        float x = td->v[i];
        float y = x;
        switch (func & 0x7) {
            case 0: y = fmaxf(0.0f, x); break;
            case 1: y = 0.5f * x * (1.0f + tanhf(0.79788456f * (x + 0.044715f * x * x * x))); break;
            case 2: y = x / (1.0f + expf(-x)); break;
            case 3: y = expf(x); break;
            case 4: y = (x == 0.0f) ? INFINITY : (1.0f / x); break;
            default: return TRAP_ILLEGAL_INSN;
        }
        td->v[i] = quantize_to_fmt(td->fmt, y);
    }
    return TRAP_NONE;
}

static Trap tensor_tcvt(Cpu *c, uint32_t d, uint32_t s, uint32_t fmt) {
    TensorReg *td = &c->tregs[d];
    TensorReg *ts = &c->tregs[s];
    if (fmt > TFMT_FP4_E2M1) return TRAP_ILLEGAL_INSN;
    TensorFmt dst = (TensorFmt)fmt;
    if (!fmt_supported(dst) || !fmt_supported(ts->fmt)) return TRAP_UNIMPLEMENTED;
    for (int i = 0; i < 256; i++) {
        float v = ts->v[i];
        td->v[i] = quantize_to_fmt(dst, v);
    }
    td->fmt = dst;
    return TRAP_NONE;
}

static Trap tensor_tred(Cpu *c, uint32_t s, uint32_t rd, uint32_t op) {
    TensorReg *ts = &c->tregs[s];
    if (!fmt_supported(ts->fmt)) return TRAP_UNIMPLEMENTED;
    if (ts->fmt == TFMT_INT8) {
        int64_t acc = (op == 1) ? INT64_MIN : (op == 2 ? INT64_MAX : 0);
        for (int i = 0; i < 256; i++) {
            int64_t v = (int64_t)(int32_t)ts->v[i];
            if (op == 0) acc += v;
            else if (op == 1) acc = (v > acc) ? v : acc;
            else if (op == 2) acc = (v < acc) ? v : acc;
            else return TRAP_ILLEGAL_INSN;
        }
        write_reg(c, rd, (uint64_t)acc);
    } else {
        double acc = (op == 1) ? -INFINITY : (op == 2 ? INFINITY : 0.0);
        for (int i = 0; i < 256; i++) {
            double v = ts->v[i];
            if (op == 0) acc += v;
            else if (op == 1) acc = (v > acc) ? v : acc;
            else if (op == 2) acc = (v < acc) ? v : acc;
            else return TRAP_ILLEGAL_INSN;
        }
        uint64_t bits;
        memcpy(&bits, &acc, sizeof(bits));
        write_reg(c, rd, bits);
    }
    return TRAP_NONE;
}

static Trap tensor_tscale(Cpu *c, uint32_t d, uint64_t rs1) {
    TensorReg *td = &c->tregs[d];
    if (!fmt_supported(td->fmt)) return TRAP_UNIMPLEMENTED;
    float s = is_float_fmt(td->fmt) ? bits_to_f32((uint32_t)rs1) : (float)(int64_t)rs1;
    for (int i = 0; i < 256; i++) {
        td->v[i] = quantize_to_fmt(td->fmt, td->v[i] * s);
    }
    return TRAP_NONE;
}

static void cap_encode(const CapReg *c, uint8_t out[16]) {
    uint64_t base = c->base;
    uint32_t len = c->len;
    uint16_t perm = c->perm;
    uint16_t otype = c->otype;
    memcpy(out + 0, &base, 8);
    memcpy(out + 8, &len, 4);
    memcpy(out + 12, &perm, 2);
    memcpy(out + 14, &otype, 2);
}

static void cap_decode(CapReg *c, const uint8_t in[16], bool tag) {
    memcpy(&c->base, in + 0, 8);
    memcpy(&c->len, in + 8, 4);
    memcpy(&c->perm, in + 12, 2);
    memcpy(&c->otype, in + 14, 2);
    c->tag = tag;
    c->sealed = false;
}

Trap cpu_step(Cpu *c, Mem *m) {
    c->regs[0] = 0;
    if (c->pc & 0x3) {
        trap_entry(c, 0, c->pc, false);
        return TRAP_NONE;
    }

    if (c->mstatus & MSTATUS_CAP) {
        uint64_t sub = 0;
        if (!cap_check(c->caps[31], c->pc, 4, 0x4, &sub)) {
            trap_entry(c, 11, sub, false);
            return TRAP_NONE;
        }
    }

    uint32_t insn = 0;
    if (!mem_read_u32(m, c->pc, &insn)) {
        trap_entry(c, 1, c->pc, false);
        return TRAP_NONE;
    }

    uint32_t opcode = get_bits(insn, 6, 0);
    uint32_t rd = rd_field(insn);
    uint32_t rs1 = rs1_field(insn);
    uint32_t rs2 = rs2_field(insn);
    uint32_t f3 = funct3(insn);
    uint32_t f7 = funct7(insn);

    uint64_t pc_next = c->pc + 4;

    if (c->trace) {
        printf("pc=0x%08llx insn=0x%08x opcode=0x%02x\n",
               (unsigned long long)c->pc, insn, opcode);
    }

    switch (opcode) {
        case OP_OP: {
            uint64_t a = c->regs[rs1];
            uint64_t b = c->regs[rs2];
            uint64_t res = 0;
            if (f7 == 0x01) {
                switch (f3) {
                    case 0x0: // mul
                        res = (uint64_t)((int64_t)a * (int64_t)b);
                        break;
                    case 0x1: { // mulh
                        __int128 prod = (__int128)(int64_t)a * (__int128)(int64_t)b;
                        res = (uint64_t)((prod >> 64) & 0xFFFFFFFFFFFFFFFFull);
                        break;
                    }
                    case 0x2: { // mulhsu
                        __int128 prod = (__int128)(int64_t)a * (__int128)(uint64_t)b;
                        res = (uint64_t)((prod >> 64) & 0xFFFFFFFFFFFFFFFFull);
                        break;
                    }
                    case 0x3: { // mulhu
                        unsigned __int128 prod = (unsigned __int128)a * (unsigned __int128)b;
                        res = (uint64_t)(prod >> 64);
                        break;
                    }
                    case 0x4: { // div
                        int64_t sa = (int64_t)a;
                        int64_t sb = (int64_t)b;
                        if (sb == 0) res = UINT64_MAX;
                        else if (sa == INT64_MIN && sb == -1) res = (uint64_t)INT64_MIN;
                        else res = (uint64_t)(sa / sb);
                        break;
                    }
                    case 0x5: { // divu
                        if (b == 0) res = UINT64_MAX;
                        else res = a / b;
                        break;
                    }
                    case 0x6: { // rem
                        int64_t sa = (int64_t)a;
                        int64_t sb = (int64_t)b;
                        if (sb == 0) res = a;
                        else if (sa == INT64_MIN && sb == -1) res = 0;
                        else res = (uint64_t)(sa % sb);
                        break;
                    }
                    case 0x7: { // remu
                        if (b == 0) res = a;
                        else res = a % b;
                        break;
                    }
                    default:
                        trap_entry(c, 2, insn, false); return TRAP_NONE;
                }
            } else {
                switch (f3) {
                    case 0x0: res = (f7 == 0x20) ? (a - b) : (a + b); break;
                    case 0x1: res = a << (b & 0x3F); break;
                    case 0x2: res = ((int64_t)a < (int64_t)b) ? 1 : 0; break;
                    case 0x3: res = (a < b) ? 1 : 0; break;
                    case 0x4: res = a ^ b; break;
                    case 0x5: res = (f7 == 0x20) ? ((int64_t)a >> (b & 0x3F)) : (a >> (b & 0x3F)); break;
                    case 0x6: res = a | b; break;
                    case 0x7: res = a & b; break;
                    default: trap_entry(c, 2, insn, false); return TRAP_NONE;
                }
            }
            write_reg(c, rd, res);
            break;
        }
        case OP_OPIMM: {
            uint64_t a = c->regs[rs1];
            int64_t imm = imm_i(insn);
            uint64_t res = 0;
            switch (f3) {
                case 0x0: res = a + (uint64_t)imm; break;
                case 0x2: res = ((int64_t)a < imm) ? 1 : 0; break;
                case 0x3: res = (a < (uint64_t)imm) ? 1 : 0; break;
                case 0x4: res = a ^ (uint64_t)imm; break;
                case 0x6: res = a | (uint64_t)imm; break;
                case 0x7: res = a & (uint64_t)imm; break;
                case 0x1:
                    if (f7 != 0x00) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                    res = a << (get_bits(insn, 25, 20) & 0x3F);
                    break;
                case 0x5: {
                    uint32_t shamt = get_bits(insn, 25, 20) & 0x3F;
                    if (f7 != 0x00 && f7 != 0x20) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                    res = (f7 == 0x20) ? ((int64_t)a >> shamt) : (a >> shamt);
                    break;
                }
                default: trap_entry(c, 2, insn, false); return TRAP_NONE;
            }
            write_reg(c, rd, res);
            break;
        }
        case OP_LOAD: {
            uint64_t addr = c->regs[rs1] + (uint64_t)imm_i(insn);
            if (addr == UART_RX_ADDR || addr == UART_STATUS_ADDR) {
                uint8_t b = 0;
                if (addr == UART_RX_ADDR) {
                    if (!uart_rx_pop(c, &b)) b = 0;
                } else {
                    b = uart_rx_status(c);
                }
                uint64_t val = 0;
                switch (f3) {
                    case 0x0: val = (int64_t)sign_extend(b, 8); break; // ldb
                    case 0x4: val = b; break; // ldbu
                    case 0x1: val = (int64_t)sign_extend(b, 8); break; // ldh
                    case 0x5: val = b; break; // ldhu
                    case 0x2: val = b; break; // ldw
                    case 0x6: val = b; break; // ldwu
                    case 0x3: val = b; break; // ld
                    default: trap_entry(c, 2, insn, false); return TRAP_NONE;
                }
                write_reg(c, rd, val);
                break;
            }
            if (c->mstatus & MSTATUS_CAP) {
                uint64_t sub = 0;
                if (!cap_check(c->caps[0], addr, 1, 0x1, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
            }
            uint64_t val = 0;
            switch (f3) {
                case 0x0: { // ldb
                    uint8_t v; if (!mem_read_u8(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = (int64_t)sign_extend(v, 8); break;
                }
                case 0x4: { // ldbu
                    uint8_t v; if (!mem_read_u8(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = v; break;
                }
                case 0x1: { // ldh
                    if (addr & 0x1) { trap_entry(c, 4, addr, false); return TRAP_NONE; }
                    uint16_t v; if (!mem_read_u16(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = (int64_t)sign_extend(v, 16); break;
                }
                case 0x5: { // ldhu
                    if (addr & 0x1) { trap_entry(c, 4, addr, false); return TRAP_NONE; }
                    uint16_t v; if (!mem_read_u16(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = v; break;
                }
                case 0x2: { // ldw
                    if (addr & 0x3) { trap_entry(c, 4, addr, false); return TRAP_NONE; }
                    uint32_t v; if (!mem_read_u32(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = (int64_t)sign_extend(v, 32); break;
                }
                case 0x6: { // ldwu
                    if (addr & 0x3) { trap_entry(c, 4, addr, false); return TRAP_NONE; }
                    uint32_t v; if (!mem_read_u32(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = v; break;
                }
                case 0x3: { // ld
                    if (addr & 0x7) { trap_entry(c, 4, addr, false); return TRAP_NONE; }
                    uint64_t v; if (!mem_read_u64(m, addr, &v)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    val = v; break;
                }
                default: trap_entry(c, 2, insn, false); return TRAP_NONE;
            }
            write_reg(c, rd, val);
            break;
        }
        case OP_STORE: {
            uint64_t addr = c->regs[rs1] + (uint64_t)imm_s(insn);
            if (c->mstatus & MSTATUS_CAP) {
                uint64_t sub = 0;
                if (!cap_check(c->caps[0], addr, 1, 0x2, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
            }
            uint64_t val = c->regs[rs2];
            if (addr == UART_TX_ADDR) {
                switch (f3) {
                    case 0x0: uart_write(val, 1); break; // stb
                    case 0x1: uart_write(val, 2); break; // sth
                    case 0x2: uart_write(val, 4); break; // stw
                    case 0x3: uart_write(val, 8); break; // st
                    default: trap_entry(c, 2, insn, false); return TRAP_NONE;
                }
                break;
            }
            switch (f3) {
                case 0x0: if (!mem_write_u8(m, addr, (uint8_t)val)) { trap_entry(c, 7, addr, false); return TRAP_NONE; } break; // stb
                case 0x1: if (addr & 0x1) { trap_entry(c, 6, addr, false); return TRAP_NONE; } if (!mem_write_u16(m, addr, (uint16_t)val)) { trap_entry(c, 7, addr, false); return TRAP_NONE; } break; // sth
                case 0x2: if (addr & 0x3) { trap_entry(c, 6, addr, false); return TRAP_NONE; } if (!mem_write_u32(m, addr, (uint32_t)val)) { trap_entry(c, 7, addr, false); return TRAP_NONE; } break; // stw
                case 0x3: if (addr & 0x7) { trap_entry(c, 6, addr, false); return TRAP_NONE; } if (!mem_write_u64(m, addr, (uint64_t)val)) { trap_entry(c, 7, addr, false); return TRAP_NONE; } break; // st
                default: trap_entry(c, 2, insn, false); return TRAP_NONE;
            }
            break;
        }
        case OP_BRANCH: {
            uint64_t a = c->regs[rs1];
            uint64_t b = c->regs[rs2];
            bool take = false;
            switch (f3) {
                case 0x0: take = (a == b); break;
                case 0x1: take = (a != b); break;
                case 0x4: take = ((int64_t)a < (int64_t)b); break;
                case 0x5: take = ((int64_t)a >= (int64_t)b); break;
                case 0x6: take = (a < b); break;
                case 0x7: take = (a >= b); break;
                default: trap_entry(c, 2, insn, false); return TRAP_NONE;
            }
            if (take) pc_next = c->pc + (uint64_t)imm_b(insn);
            break;
        }
        case OP_JAL: {
            write_reg(c, rd, c->pc + 4);
            pc_next = c->pc + (uint64_t)imm_j(insn);
            break;
        }
        case OP_JALR: {
            write_reg(c, rd, c->pc + 4);
            pc_next = (c->regs[rs1] + (uint64_t)imm_i(insn)) & ~0x3ull;
            break;
        }
        case OP_MOVHI: {
            write_reg(c, rd, (uint64_t)imm_u(insn));
            break;
        }
        case OP_MOVPC: {
            write_reg(c, rd, c->pc + (uint64_t)imm_u(insn));
            break;
        }
        case OP_FENCE:
            break;
        case OP_SYSTEM: {
            uint32_t imm = get_bits(insn, 31, 20);
            if (f3 == 0 && rd == 0 && rs1 == 0) {
                if (imm == 0x000) {
                    Trap t = syscall_handle(c, m);
                    if (t == TRAP_NONE) break;
                    if (t == TRAP_EBREAK) return TRAP_EBREAK;
                    uint64_t cause = (c->mode == MODE_U) ? 8 : (c->mode == MODE_S ? 9 : 10);
                    trap_entry(c, cause, 0, false);
                    return TRAP_NONE;
                }
                if (imm == 0x001) {
                    return TRAP_EBREAK;
                }
                if (imm == 0x302) { // mret
                    uint64_t mpp = (c->mstatus & MSTATUS_MPP_MASK) >> MSTATUS_MPP_SHIFT;
                    c->mode = (PrivMode)mpp;
                    c->mstatus = (c->mstatus & ~MSTATUS_MPP_MASK);
                    uint64_t mie = (c->mstatus & MSTATUS_MPIE) ? 1 : 0;
                    if (mie) c->mstatus |= MSTATUS_MIE; else c->mstatus &= ~MSTATUS_MIE;
                    c->mstatus |= MSTATUS_MPIE;
                    c->pc = c->mepc;
                    pc_next = c->pc;
                    break;
                }
                if (imm == 0x102) { // sret
                    uint64_t spp = (c->mstatus & MSTATUS_SPP) ? 1 : 0;
                    c->mode = spp ? MODE_S : MODE_U;
                    c->mstatus &= ~MSTATUS_SPP;
                    uint64_t sie = (c->mstatus & MSTATUS_SPIE) ? 1 : 0;
                    if (sie) c->mstatus |= MSTATUS_SIE; else c->mstatus &= ~MSTATUS_SIE;
                    c->mstatus |= MSTATUS_SPIE;
                    c->pc = c->sepc;
                    pc_next = c->pc;
                    break;
                }
            }
            if (f3 >= 1 && f3 <= 3) {
                uint32_t csr = get_bits(insn, 31, 20);
                if (!csr_access_ok(c, csr)) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                uint64_t old = 0;
                if (!csr_read(c, csr, &old)) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                uint64_t val = c->regs[rs1];
                bool write_attempt = (f3 == 1) || (rs1 != 0);
                bool write_ok = true;
                if (write_attempt) {
                    if (f3 == 1) write_ok = csr_write(c, csr, val);
                    else if (f3 == 2) write_ok = csr_write(c, csr, old | val);
                    else if (f3 == 3) write_ok = csr_write(c, csr, old & ~val);
                    if (!write_ok) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                }
                write_reg(c, rd, old);
                break;
            }
            if (f3 >= 5 && f3 <= 7) {
                uint32_t csr = get_bits(insn, 31, 20);
                if (!csr_access_ok(c, csr)) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                uint64_t old = 0;
                if (!csr_read(c, csr, &old)) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                uint64_t zimm = rs1 & 0x1F;
                bool write_attempt = (f3 == 5) || (zimm != 0);
                bool write_ok = true;
                if (write_attempt) {
                    if (f3 == 5) write_ok = csr_write(c, csr, zimm);
                    else if (f3 == 6) write_ok = csr_write(c, csr, old | zimm);
                    else if (f3 == 7) write_ok = csr_write(c, csr, old & ~zimm);
                    if (!write_ok) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                }
                write_reg(c, rd, old);
                break;
            }
            trap_entry(c, 2, insn, false);
            return TRAP_NONE;
        }
        case OP_CAP: {
            if (!(c->mstatus & MSTATUS_CAP)) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
            if (f7 == 0x00 && (f3 >= 0x2 || rs2 != 0)) {
                switch (f3) {
                    case 0x0: { // csetbounds
                        CapReg src = c->caps[rs1];
                        uint64_t len = c->regs[rs2];
                        if (len < src.len) src.len = (uint32_t)len;
                        c->caps[rd] = src;
                        break;
                    }
                    case 0x1: { // csetperm
                        CapReg src = c->caps[rs1];
                        src.perm = (uint16_t)c->regs[rs2];
                        c->caps[rd] = src;
                        break;
                    }
                    case 0x2: { // cseal
                        CapReg src = c->caps[rs1];
                        src.sealed = true;
                        src.otype = (uint16_t)c->regs[rs2];
                        c->caps[rd] = src;
                        break;
                    }
                    case 0x3: { // cunseal
                        CapReg src = c->caps[rs1];
                        if (!src.sealed || src.otype != (uint16_t)c->regs[rs2]) { trap_entry(c, 11, 0x4, false); return TRAP_NONE; }
                        src.sealed = false;
                        c->caps[rd] = src;
                        break;
                    }
                    case 0x4: // cgettag
                        write_reg(c, rd, c->caps[rs1].tag ? 1 : 0);
                        break;
                    case 0x5: // cgetbase
                        write_reg(c, rd, c->caps[rs1].base);
                        break;
                    case 0x6: // cgetlen
                        write_reg(c, rd, c->caps[rs1].len);
                        break;
                    default:
                        trap_entry(c, 2, insn, false);
                        return TRAP_NONE;
                }
                break;
            }
            if (f3 == 0x0) { // cld
                uint64_t addr = c->regs[rs1] + (uint64_t)imm_i(insn);
                uint64_t sub = 0;
                if (!cap_check(c->caps[0], addr, 16, 0x1, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                uint8_t buf[16]; bool tag;
                if (!mem_read_cap(m, addr, buf, &tag)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                cap_decode(&c->caps[rd], buf, tag);
                break;
            }
            if (f3 == 0x1) { // cst
                uint64_t addr = c->regs[rs1] + (uint64_t)imm_i(insn);
                uint64_t sub = 0;
                if (!cap_check(c->caps[0], addr, 16, 0x2, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                uint8_t buf[16];
                cap_encode(&c->caps[rd], buf);
                if (!mem_write_cap(m, addr, buf, c->caps[rd].tag)) { trap_entry(c, 7, addr, false); return TRAP_NONE; }
                break;
            }
            trap_entry(c, 2, insn, false);
            return TRAP_NONE;
        }
        case OP_TENSOR: {
            uint32_t trd = rd & 0x7;
            uint32_t trs1 = rs1 & 0x7;
            uint32_t trs2 = rs2 & 0x7;

            bool rtype = (rs1 < 8) && (rs2 < 8);
            if (rtype && f3 == 0x0 && f7 == 0x00) { // tadd
                Trap t = tensor_tadd(c, trd, trs1, trs2); if (t != TRAP_NONE) return t; break;
            }
            if (rtype && f3 == 0x1 && f7 == 0x01) { // tmma
                Trap t = tensor_tmma(c, trd, trs1, trs2); if (t != TRAP_NONE) return t; break;
            }

            if (f3 == 0x0) { // tld
                int64_t stride = imm_i(insn);
                uint64_t base = c->regs[rs1];
                if (c->mstatus & MSTATUS_CAP) {
                    uint64_t sub = 0;
                    if (!cap_check(c->caps[0], base, 16, 0x1, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                }
                Trap t = tensor_tld(c, m, trd, base, stride); if (t != TRAP_NONE) return t; break;
            }
            if (f3 == 0x1) { // tst
                int64_t stride = imm_i(insn);
                uint64_t base = c->regs[rs1];
                if (c->mstatus & MSTATUS_CAP) {
                    uint64_t sub = 0;
                    if (!cap_check(c->caps[0], base, 16, 0x2, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                }
                Trap t = tensor_tst(c, m, trd, base, stride); if (t != TRAP_NONE) return t; break;
            }
            if (f3 == 0x2) { // tact
                if (rs1 != rd) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                uint32_t imm = (uint32_t)imm_i(insn) & 0x7;
                Trap t = tensor_tact(c, trd, imm); if (t != TRAP_NONE) return t; break;
            }
            if (f3 == 0x3) { // tcvt
                uint32_t imm = (uint32_t)imm_i(insn) & 0xF;
                Trap t = tensor_tcvt(c, trd, trs1, imm); if (t != TRAP_NONE) return t; break;
            }
            if (f3 == 0x4) { // tzero
                if (rs1 != rd) { trap_entry(c, 2, insn, false); return TRAP_NONE; }
                for (int i = 0; i < 256; i++) c->tregs[trd].v[i] = 0.0f;
                break;
            }
            if (f3 == 0x5) { // tred
                uint32_t imm = (uint32_t)imm_i(insn) & 0x3;
                Trap t = tensor_tred(c, trs1, rd, imm); if (t != TRAP_NONE) return t; break;
            }
            if (f3 == 0x6) { // tscale
                Trap t = tensor_tscale(c, trd, c->regs[rs1]); if (t != TRAP_NONE) return t; break;
            }
            trap_entry(c, 2, insn, false);
            return TRAP_NONE;
        }
        case OP_AMO: {
            uint64_t addr = c->regs[rs1];
            if (f7 == 0x04) { // amoswap
                if (f3 == 0x2) { // amoswap.w
                    if (addr & 0x3) { trap_entry(c, 6, addr, false); return TRAP_NONE; }
                    if (c->mstatus & MSTATUS_CAP) {
                        uint64_t sub = 0;
                        if (!cap_check(c->caps[0], addr, 4, 0x3, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                    }
                    uint32_t oldv = 0;
                    if (!mem_read_u32(m, addr, &oldv)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    uint32_t newv = (uint32_t)c->regs[rs2];
                    if (!mem_write_u32(m, addr, newv)) { trap_entry(c, 7, addr, false); return TRAP_NONE; }
                    write_reg(c, rd, (uint64_t)sign_extend(oldv, 32));
                    break;
                }
                if (f3 == 0x3) { // amoswap.d
                    if (addr & 0x7) { trap_entry(c, 6, addr, false); return TRAP_NONE; }
                    if (c->mstatus & MSTATUS_CAP) {
                        uint64_t sub = 0;
                        if (!cap_check(c->caps[0], addr, 8, 0x3, &sub)) { trap_entry(c, 11, sub, false); return TRAP_NONE; }
                    }
                    uint64_t oldv = 0;
                    if (!mem_read_u64(m, addr, &oldv)) { trap_entry(c, 5, addr, false); return TRAP_NONE; }
                    uint64_t newv = c->regs[rs2];
                    if (!mem_write_u64(m, addr, newv)) { trap_entry(c, 7, addr, false); return TRAP_NONE; }
                    write_reg(c, rd, oldv);
                    break;
                }
            }
            trap_entry(c, 2, insn, false);
            return TRAP_NONE;
        }
        default:
            trap_entry(c, 2, insn, false);
            return TRAP_NONE;
    }

    c->pc = pc_next;
    c->steps++;
    c->cycle++;
    c->instret++;
    c->time = c->cycle;

    if (c->dump_regs) cpu_dump_regs(c);

    return TRAP_NONE;
}
