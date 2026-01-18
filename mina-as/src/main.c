#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 32
#define MAX_LABELS 1024
#define MAX_LINE 512

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_EXEC 2
#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

typedef struct {
    char name[64];
    uint64_t addr;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    size_t count;
} LabelTable;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t cap;
} Buffer;

typedef enum {
    SEC_TEXT = 0,
    SEC_DATA = 1,
    SEC_BSS = 2,
} SectionKind;

typedef struct {
    Buffer buf;
    uint64_t pc;
    uint64_t base;
} Section;

typedef struct {
    uint64_t text_base;
    uint64_t data_base;
    uint64_t bss_base;
    uint64_t seg_align;
} AsmOptions;

static void buf_init(Buffer *b) {
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
}

static void buf_reserve(Buffer *b, size_t n) {
    if (b->size + n <= b->cap) return;
    size_t newcap = b->cap ? b->cap * 2 : 4096;
    while (newcap < b->size + n) newcap *= 2;
    b->data = (uint8_t *)realloc(b->data, newcap);
    b->cap = newcap;
}

static void buf_write(Buffer *b, const void *p, size_t n) {
    buf_reserve(b, n);
    memcpy(b->data + b->size, p, n);
    b->size += n;
}

static void buf_write_u8(Buffer *b, uint8_t v) { buf_write(b, &v, 1); }
static void buf_write_u16(Buffer *b, uint16_t v) { buf_write(b, &v, 2); }
static void buf_write_u32(Buffer *b, uint32_t v) { buf_write(b, &v, 4); }
static void buf_write_u64(Buffer *b, uint64_t v) { buf_write(b, &v, 8); }

static int ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    if (su > sl) return 0;
    return strcmp(s + sl - su, suffix) == 0;
}

static uint64_t align_up(uint64_t v, uint64_t a) {
    if (a == 0) return v;
    uint64_t m = a - 1;
    return (v + m) & ~m;
}

static int section_from_name(const char *s, SectionKind *out) {
    if (!s) return 0;
    const char *name = s;
    if (name[0] == '.') name++;
    if (strcmp(name, "text") == 0) { *out = SEC_TEXT; return 1; }
    if (strcmp(name, "data") == 0) { *out = SEC_DATA; return 1; }
    if (strcmp(name, "bss") == 0) { *out = SEC_BSS; return 1; }
    return 0;
}

static int write_elf_file_sections(const char *out_path, const Section *text, const Section *data, const Section *bss, uint64_t entry, uint64_t seg_align) {
    FILE *out = fopen(out_path, "wb");
    if (!out) return 0;

    Elf64_Phdr phdrs[3];
    uint16_t phnum = 0;

    if (text->buf.size > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_X;
        ph.p_vaddr = text->base;
        ph.p_paddr = text->base;
        ph.p_filesz = text->buf.size;
        ph.p_memsz = text->buf.size;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    if (data->buf.size > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_W;
        ph.p_vaddr = data->base;
        ph.p_paddr = data->base;
        ph.p_filesz = data->buf.size;
        ph.p_memsz = data->buf.size;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    if (bss->pc > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_W;
        ph.p_vaddr = bss->base;
        ph.p_paddr = bss->base;
        ph.p_filesz = 0;
        ph.p_memsz = bss->pc;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    Elf64_Ehdr eh = {0};
    eh.e_ident[0] = ELF_MAGIC0;
    eh.e_ident[1] = ELF_MAGIC1;
    eh.e_ident[2] = ELF_MAGIC2;
    eh.e_ident[3] = ELF_MAGIC3;
    eh.e_ident[4] = ELFCLASS64;
    eh.e_ident[5] = ELFDATA2LSB;
    eh.e_ident[6] = EV_CURRENT;
    eh.e_type = ET_EXEC;
    eh.e_machine = 0;
    eh.e_version = EV_CURRENT;
    eh.e_entry = entry;
    eh.e_phoff = sizeof(Elf64_Ehdr);
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum = phnum;

    uint64_t file_off = sizeof(Elf64_Ehdr) + (uint64_t)phnum * sizeof(Elf64_Phdr);
    uint64_t cur = align_up(file_off, seg_align);

    for (uint16_t i = 0; i < phnum; i++) {
        if (phdrs[i].p_filesz == 0) {
            phdrs[i].p_offset = 0;
            continue;
        }
        phdrs[i].p_offset = cur;
        cur = align_up(cur + phdrs[i].p_filesz, seg_align);
    }

    if (fwrite(&eh, 1, sizeof(eh), out) != sizeof(eh)) { fclose(out); return 0; }
    if (phnum > 0 && fwrite(phdrs, sizeof(Elf64_Phdr), phnum, out) != phnum) { fclose(out); return 0; }

    long pos = ftell(out);
    if (pos < 0) { fclose(out); return 0; }
    if ((uint64_t)pos > file_off) { fclose(out); return 0; }

    uint64_t pad = align_up((uint64_t)pos, seg_align) - (uint64_t)pos;
    for (uint64_t i = 0; i < pad; i++) fputc(0, out);

    for (uint16_t i = 0; i < phnum; i++) {
        if (phdrs[i].p_filesz == 0) continue;
        const uint8_t *buf = NULL;
        size_t size = 0;
        if (phdrs[i].p_vaddr == text->base) { buf = text->buf.data; size = text->buf.size; }
        else if (phdrs[i].p_vaddr == data->base) { buf = data->buf.data; size = data->buf.size; }
        if (!buf || size != phdrs[i].p_filesz) { fclose(out); return 0; }

        long curpos = ftell(out);
        if (curpos < 0) { fclose(out); return 0; }
        if ((uint64_t)curpos < phdrs[i].p_offset) {
            uint64_t pad2 = phdrs[i].p_offset - (uint64_t)curpos;
            for (uint64_t p = 0; p < pad2; p++) fputc(0, out);
        }
        if (size > 0 && fwrite(buf, 1, size, out) != size) { fclose(out); return 0; }
    }

    fclose(out);
    return 1;
}

static void label_add(LabelTable *t, const char *name, uint64_t addr) {
    for (size_t i = 0; i < t->count; i++) {
        if (strcmp(t->labels[i].name, name) == 0) {
            t->labels[i].addr = addr;
            return;
        }
    }
    if (t->count < MAX_LABELS) {
        strncpy(t->labels[t->count].name, name, sizeof(t->labels[t->count].name) - 1);
        t->labels[t->count].addr = addr;
        t->count++;
    }
}

static int label_find(const LabelTable *t, const char *name, uint64_t *out) {
    for (size_t i = 0; i < t->count; i++) {
        if (strcmp(t->labels[i].name, name) == 0) {
            *out = t->labels[i].addr;
            return 1;
        }
    }
    return 0;
}

static void strip_comment(char *line) {
    for (char *p = line; *p; p++) {
        if (*p == '#' || *p == ';') {
            *p = '\0';
            break;
        }
    }
}

static int is_label_def(const char *s) {
    size_t n = strlen(s);
    return n > 0 && s[n - 1] == ':';
}

static int tokenize(char *line, char *tokens[], int max_tokens) {
    int count = 0;
    for (char *p = line; *p;) {
        while (isspace((unsigned char)*p) || *p == ',') p++;
        if (*p == '\0') break;
        if (*p == '(' || *p == ')') { p++; continue; }
        if (count >= max_tokens) break;
        if (*p == '"') {
            tokens[count++] = p;
            p++;
            while (*p) {
                if (*p == '\\' && p[1] != '\0') { p += 2; continue; }
                if (*p == '"') { p++; break; }
                p++;
            }
            if (*p) { *p = '\0'; p++; }
            continue;
        }
        tokens[count++] = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != '(' && *p != ')') p++;
        if (*p) { *p = '\0'; p++; }
    }
    return count;
}

static int parse_int(const char *s, int64_t *out) {
    if (!s || !*s) return 0;
    char *end = NULL;
    int64_t v = strtoll(s, &end, 0);
    if (end && *end == '\0') { *out = v; return 1; }
    return 0;
}

static int parse_reg(const char *s) {
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

static int parse_creg(const char *s) {
    if (!s) return -1;
    if (s[0] == 'c' && isdigit((unsigned char)s[1])) {
        int n = atoi(s + 1);
        return (n >= 0 && n < 32) ? n : -1;
    }
    return -1;
}

static int parse_treg(const char *s) {
    if (!s) return -1;
    if (s[0] == 't' && s[1] == 'r' && isdigit((unsigned char)s[2])) {
        int n = atoi(s + 2);
        return (n >= 0 && n < 8) ? n : -1;
    }
    return -1;
}

static uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    return (uimm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0xFFF);
    uint32_t imm_hi = (uimm >> 5) & 0x7F;
    uint32_t imm_lo = uimm & 0x1F;
    return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_lo << 7) | opcode;
}

static uint32_t encode_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0x1FFF);
    uint32_t bit12 = (uimm >> 12) & 1;
    uint32_t bit11 = (uimm >> 11) & 1;
    uint32_t bits10_5 = (uimm >> 5) & 0x3F;
    uint32_t bits4_1 = (uimm >> 1) & 0xF;
    return (bit12 << 31) | (bits10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (bits4_1 << 8) | (bit11 << 7) | opcode;
}

static uint32_t encode_u(int32_t imm20, uint32_t rd, uint32_t opcode) {
    return ((uint32_t)imm20 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_j(int32_t imm, uint32_t rd, uint32_t opcode) {
    uint32_t uimm = (uint32_t)(imm & 0x1FFFFF);
    uint32_t bit20 = (uimm >> 20) & 1;
    uint32_t bits10_1 = (uimm >> 1) & 0x3FF;
    uint32_t bit11 = (uimm >> 11) & 1;
    uint32_t bits19_12 = (uimm >> 12) & 0xFF;
    return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) | (rd << 7) | opcode;
}

static int64_t resolve_imm(const char *s, const LabelTable *labels, uint64_t pc, int is_pc_rel) {
    int64_t v = 0;
    if (parse_int(s, &v)) return v;
    uint64_t addr = 0;
    if (!label_find(labels, s, &addr)) return 0;
    if (is_pc_rel) return (int64_t)(addr - pc);
    return (int64_t)addr;
}

static int emit_directive(Section *sec, SectionKind kind, char *tokens[], int count) {
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
        uint64_t mask = (1ull << v) - 1ull;
        while ((sec->pc & mask) != 0) {
            if (kind != SEC_BSS) buf_write_u8(&sec->buf, 0);
            sec->pc++;
        }
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

static uint32_t csr_addr_from_name(const char *s) {
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

static uint32_t tensor_func_from_name(const char *s) {
    if (strcmp(s, "relu") == 0) return 0;
    if (strcmp(s, "gelu") == 0) return 1;
    if (strcmp(s, "silu") == 0) return 2;
    if (strcmp(s, "exp") == 0) return 3;
    if (strcmp(s, "recip") == 0) return 4;
    return 0xFFFFFFFFu;
}

static uint32_t tensor_fmt_from_name(const char *s) {
    if (strcmp(s, "fp32") == 0) return 0;
    if (strcmp(s, "fp16") == 0) return 1;
    if (strcmp(s, "bf16") == 0) return 2;
    if (strcmp(s, "fp8e4m3") == 0) return 3;
    if (strcmp(s, "fp8e5m2") == 0) return 4;
    if (strcmp(s, "int8") == 0) return 5;
    if (strcmp(s, "fp4e2m1") == 0) return 6;
    return 0xFFFFFFFFu;
}

static uint32_t tensor_redop_from_name(const char *s) {
    if (strcmp(s, "sum") == 0) return 0;
    if (strcmp(s, "max") == 0) return 1;
    if (strcmp(s, "min") == 0) return 2;
    return 0xFFFFFFFFu;
}

static int assemble_line(Section *sec, SectionKind kind, char *tokens[], int count, const LabelTable *labels) {
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

static int first_pass(FILE *f, LabelTable *labels, const AsmOptions *opt) {
    char line[MAX_LINE];
    SectionKind cur = SEC_TEXT;
    uint64_t text_pc = 0, data_pc = 0, bss_pc = 0;
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
            if (count == 0) continue;
        }

        if (tokens[0][0] == '.') {
            SectionKind next;
            if (strcmp(tokens[0], ".text") == 0) { cur = SEC_TEXT; continue; }
            if (strcmp(tokens[0], ".data") == 0) { cur = SEC_DATA; continue; }
            if (strcmp(tokens[0], ".bss") == 0) { cur = SEC_BSS; continue; }
            if (strcmp(tokens[0], ".section") == 0 && count >= 2 && section_from_name(tokens[1], &next)) { cur = next; continue; }

            uint64_t *pc = (cur == SEC_TEXT) ? &text_pc : (cur == SEC_DATA ? &data_pc : &bss_pc);
            uint64_t base = (cur == SEC_TEXT) ? opt->text_base : (cur == SEC_DATA ? opt->data_base : opt->bss_base);

            if (strcmp(tokens[0], ".org") == 0 && count >= 2) {
                int64_t v; if (!parse_int(tokens[1], &v)) return 0;
                if ((uint64_t)v < base) return 0;
                *pc = (uint64_t)v - base;
            } else if (strcmp(tokens[0], ".align") == 0 && count >= 2) {
                int64_t v; if (!parse_int(tokens[1], &v)) return 0;
                uint64_t mask = (1ull << v) - 1ull;
                while ((*pc & mask) != 0) (*pc)++;
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
        }
    }
    return 1;
}

static int assemble_file(const char *in_path, const char *out_path, bool elf_output, const AsmOptions *opt) {
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
    while (fgets(line, sizeof(line), f)) {
        strip_comment(line);
        char *tokens[MAX_TOKENS];
        int count = tokenize(line, tokens, MAX_TOKENS);
        if (count == 0) continue;
        if (is_label_def(tokens[0])) {
            tokens[0][strlen(tokens[0]) - 1] = '\0';
            for (int i = 1; i < count; i++) tokens[i - 1] = tokens[i];
            count--;
            if (count == 0) continue;
        }
        if (tokens[0][0] == '.') {
            SectionKind next;
            if (strcmp(tokens[0], ".text") == 0) { cur = SEC_TEXT; continue; }
            if (strcmp(tokens[0], ".data") == 0) { cur = SEC_DATA; continue; }
            if (strcmp(tokens[0], ".bss") == 0) { cur = SEC_BSS; continue; }
            if (strcmp(tokens[0], ".section") == 0 && count >= 2 && section_from_name(tokens[1], &next)) { cur = next; continue; }
        }

        Section *sec = (cur == SEC_TEXT) ? &text : (cur == SEC_DATA ? &data : &bss);
        if (!assemble_line(sec, cur, tokens, count, &labels)) {
            uint64_t abs_pc = sec->base + sec->pc;
            fprintf(stderr, "assemble error at pc=0x%llx\n", (unsigned long long)abs_pc);
            ok = 0;
            break;
        }
    }
    fclose(f);

    if (!ok) { free(text.buf.data); free(data.buf.data); return 0; }

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

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = "a.out.elf";
    bool elf_output = true;
    AsmOptions opt = {
        .text_base = 0x0,
        .data_base = 0x1000,
        .bss_base = 0x2000,
        .seg_align = 0x1000,
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--elf") == 0 || strcmp(argv[i], "-elf") == 0) {
            elf_output = true;
        } else if (strcmp(argv[i], "--bin") == 0 || strcmp(argv[i], "-bin") == 0) {
            elf_output = false;
        } else if (strcmp(argv[i], "--text-base") == 0 && i + 1 < argc) {
            opt.text_base = strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--data-base") == 0 && i + 1 < argc) {
            opt.data_base = strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--bss-base") == 0 && i + 1 < argc) {
            opt.bss_base = strtoull(argv[++i], NULL, 0);
        } else if ((strcmp(argv[i], "--segment-align") == 0 || strcmp(argv[i], "--seg-align") == 0) && i + 1 < argc) {
            opt.seg_align = strtoull(argv[++i], NULL, 0);
        } else if (!in_path) {
            in_path = argv[i];
        } else {
            fprintf(stderr, "usage: %s [--elf|--bin] [--text-base N] [--data-base N] [--bss-base N] [--segment-align N] input.s -o output.elf\n", argv[0]);
            return 1;
        }
    }

    if (!in_path) {
        fprintf(stderr, "usage: %s [--elf|--bin] [--text-base N] [--data-base N] [--bss-base N] [--segment-align N] input.s -o output.elf\n", argv[0]);
        return 1;
    }

    if (elf_output && ends_with(out_path, ".bin")) elf_output = false;

    if (!assemble_file(in_path, out_path, elf_output, &opt)) {
        fprintf(stderr, "assembly failed\n");
        return 1;
    }

    return 0;
}
