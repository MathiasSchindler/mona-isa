#include "asm.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int parse_int(const char *s, int64_t *out) {
    if (!s || !*s) return 0;
    char *end = NULL;
    int64_t v = strtoll(s, &end, 0);
    if (end && *end == '\0') { *out = v; return 1; }
    return 0;
}

int parse_reg(const char *s) {
    if (!s) return -1;
    if (s[0] == 'r' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        return (n >= 0 && n < 32) ? n : -1;
    }
    if (strcmp(s, "zero") == 0) return 0;
    if (strcmp(s, "ra") == 0) return 31;
    if (strcmp(s, "sp") == 0) return 30;
    if (strcmp(s, "gp") == 0) return 29;
    if (strcmp(s, "fp") == 0) return 28;
    if (strcmp(s, "tp") == 0) return 15;
    if (s[0] == 'a' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        if (n >= 0 && n <= 7) return 16 + n;
    }
    if (s[0] == 't' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        if (n >= 0 && n <= 3) return 24 + n;
        if (n >= 4 && n <= 10) return 8 + (n - 4);
    }
    if (s[0] == 's' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        if (n >= 0 && n <= 6) return 1 + n;
    }
    return -1;
}

int parse_creg(const char *s) {
    if (!s) return -1;
    if (s[0] == 'c' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        return (n >= 0 && n < 32) ? n : -1;
    }
    return -1;
}

int parse_treg(const char *s) {
    if (!s) return -1;
    if (s[0] == 't' && s[1] == 'r' && isdigit((unsigned char)s[2])) {
        int n = atoi(s + 2);
        return (n >= 0 && n < 8) ? n : -1;
    }
    return -1;
}

int64_t resolve_imm(const char *s, const LabelTable *labels, uint64_t pc, int is_pc_rel) {
    int64_t v = 0;
    if (parse_int(s, &v)) return v;
    uint64_t addr = 0;
    if (!label_find(labels, s, &addr)) return 0;
    if (is_pc_rel) return (int64_t)(addr - pc);
    return (int64_t)addr;
}

uint32_t csr_addr_from_name(const char *s) {
    if (!s) return 0xFFFFFFFFu;
    if (strcmp(s, "mstatus") == 0) return 0x300;
    if (strcmp(s, "mie") == 0) return 0x304;
    if (strcmp(s, "medeleg") == 0) return 0x302;
    if (strcmp(s, "mideleg") == 0) return 0x303;
    if (strcmp(s, "mtvec") == 0) return 0x305;
    if (strcmp(s, "mip") == 0) return 0x344;
    if (strcmp(s, "mscratch") == 0) return 0x340;
    if (strcmp(s, "mepc") == 0) return 0x341;
    if (strcmp(s, "mcause") == 0) return 0x342;
    if (strcmp(s, "mtval") == 0) return 0x343;
    if (strcmp(s, "sstatus") == 0) return 0x100;
    if (strcmp(s, "sie") == 0) return 0x104;
    if (strcmp(s, "stvec") == 0) return 0x105;
    if (strcmp(s, "sip") == 0) return 0x144;
    if (strcmp(s, "sscratch") == 0) return 0x140;
    if (strcmp(s, "sepc") == 0) return 0x141;
    if (strcmp(s, "scause") == 0) return 0x142;
    if (strcmp(s, "stval") == 0) return 0x143;
    return 0xFFFFFFFFu;
}

uint32_t tensor_func_from_name(const char *s) {
    if (strcmp(s, "relu") == 0) return 0;
    if (strcmp(s, "gelu") == 0) return 1;
    if (strcmp(s, "silu") == 0) return 2;
    if (strcmp(s, "exp") == 0) return 3;
    if (strcmp(s, "recip") == 0) return 4;
    return 0xFFFFFFFFu;
}

uint32_t tensor_fmt_from_name(const char *s) {
    if (strcmp(s, "fp32") == 0) return 0;
    if (strcmp(s, "fp16") == 0) return 1;
    if (strcmp(s, "bf16") == 0) return 2;
    if (strcmp(s, "fp8e4m3") == 0) return 3;
    if (strcmp(s, "fp8e5m2") == 0) return 4;
    if (strcmp(s, "int8") == 0) return 5;
    if (strcmp(s, "fp4e2m1") == 0) return 6;
    return 0xFFFFFFFFu;
}

uint32_t tensor_redop_from_name(const char *s) {
    if (strcmp(s, "sum") == 0) return 0;
    if (strcmp(s, "max") == 0) return 1;
    if (strcmp(s, "min") == 0) return 2;
    return 0xFFFFFFFFu;
}
