#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_DYNSYM 11

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

typedef struct {
    int stop_at_ebreak;
    int only_text;
    int stats;
    int json;
    const char *path;
} Options;

static int read_exact(FILE *f, void *buf, size_t len);

static char *dup_string(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}

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

static int32_t sign_extend(uint32_t value, int bits) {
    uint32_t mask = 1u << (bits - 1);
    return (int32_t)((value ^ mask) - mask);
}

static const char *reg_name(unsigned reg) {
    static const char *names[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
        "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
    };
    if (reg < 32) return names[reg];
    return "r?";
}

static void fmt_r(char *buf, size_t len, const char *mn, uint32_t rd, uint32_t rs1, uint32_t rs2) {
    snprintf(buf, len, "%s %s, %s, %s", mn, reg_name(rd), reg_name(rs1), reg_name(rs2));
}

static void fmt_i(char *buf, size_t len, const char *mn, uint32_t rd, uint32_t rs1, int32_t imm) {
    snprintf(buf, len, "%s %s, %s, %d", mn, reg_name(rd), reg_name(rs1), imm);
}

static void fmt_load(char *buf, size_t len, const char *mn, uint32_t rd, uint32_t rs1, int32_t imm) {
    snprintf(buf, len, "%s %s, %d(%s)", mn, reg_name(rd), imm, reg_name(rs1));
}

static void fmt_store(char *buf, size_t len, const char *mn, uint32_t rs2, uint32_t rs1, int32_t imm) {
    snprintf(buf, len, "%s %s, %d(%s)", mn, reg_name(rs2), imm, reg_name(rs1));
}

static void fmt_branch(char *buf, size_t len, const char *mn, uint32_t rs1, uint32_t rs2, int32_t imm, uint64_t pc) {
    snprintf(buf, len, "%s %s, %s, 0x%llx", mn, reg_name(rs1), reg_name(rs2), (unsigned long long)(pc + imm));
}

typedef struct {
    const char *mnemonic;
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    int32_t imm;
    int has_imm;
    int is_branch;
    int is_jump;
    int is_conditional;
    int has_target;
    uint64_t target;
    char text[128];
} Decoded;

typedef struct {
    uint64_t addr;
    int id;
} Label;

typedef struct {
    uint64_t *items;
    size_t count;
    size_t cap;
} AddrList;

typedef struct {
    uint64_t from;
    uint64_t to;
    int fallthrough;
} Edge;

typedef struct {
    const char *name;
    int count;
} MnemonicCount;

static void json_escape(FILE *out, const char *s) {
    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            fputc('\\', out);
            fputc(c, out);
        } else if (c == '\n') {
            fputs("\\n", out);
        } else if (c == '\r') {
            fputs("\\r", out);
        } else if (c == '\t') {
            fputs("\\t", out);
        } else if (c < 0x20) {
            fprintf(out, "\\u%04x", c);
        } else {
            fputc(c, out);
        }
    }
}

static void usage(void) {
    fprintf(stderr, "usage: mina-elf-info [--stop-at-ebreak] [--only-text] [--stats] [--json] <file>\n");
}

static int parse_options(int argc, char **argv, Options *opt) {
    memset(opt, 0, sizeof(*opt));
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--stop-at-ebreak") == 0) { opt->stop_at_ebreak = 1; continue; }
        if (strcmp(arg, "--only-text") == 0) { opt->only_text = 1; continue; }
        if (strcmp(arg, "--stats") == 0) { opt->stats = 1; continue; }
        if (strcmp(arg, "--json") == 0) { opt->json = 1; continue; }
        if (arg[0] == '-') return 0;
        opt->path = arg;
    }
    return opt->path != NULL;
}

static int label_find(const Label *labels, size_t count, uint64_t addr) {
    for (size_t i = 0; i < count; i++) {
        if (labels[i].addr == addr) return labels[i].id;
    }
    return -1;
}

static int label_add(Label **labels, size_t *count, size_t *cap, uint64_t addr) {
    int id = label_find(*labels, *count, addr);
    if (id >= 0) return id;
    if (*count + 1 > *cap) {
        size_t next = (*cap == 0) ? 8 : (*cap * 2);
        Label *mem = (Label *)realloc(*labels, next * sizeof(Label));
        if (!mem) return -1;
        *labels = mem;
        *cap = next;
    }
    id = (int)(*count);
    (*labels)[*count].addr = addr;
    (*labels)[*count].id = id;
    (*count)++;
    return id;
}

static int addrlist_add_unique(AddrList *list, uint64_t addr) {
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i] == addr) return 1;
    }
    if (list->count + 1 > list->cap) {
        size_t next = (list->cap == 0) ? 8 : (list->cap * 2);
        uint64_t *mem = (uint64_t *)realloc(list->items, next * sizeof(uint64_t));
        if (!mem) return 0;
        list->items = mem;
        list->cap = next;
    }
    list->items[list->count++] = addr;
    return 1;
}

static int edge_add(Edge **edges, size_t *count, size_t *cap, uint64_t from, uint64_t to, int fallthrough) {
    if (*count + 1 > *cap) {
        size_t next = (*cap == 0) ? 8 : (*cap * 2);
        Edge *mem = (Edge *)realloc(*edges, next * sizeof(Edge));
        if (!mem) return 0;
        *edges = mem;
        *cap = next;
    }
    (*edges)[*count].from = from;
    (*edges)[*count].to = to;
    (*edges)[*count].fallthrough = fallthrough;
    (*count)++;
    return 1;
}

typedef struct {
    char *name;
    uint64_t value;
    uint64_t size;
    unsigned char info;
} Symbol;

static void symbols_free(Symbol *syms, size_t count) {
    for (size_t i = 0; i < count; i++) free(syms[i].name);
    free(syms);
}

static int read_symbols(FILE *f, const Elf64_Ehdr *eh, Symbol **out_syms, size_t *out_count) {
    *out_syms = NULL;
    *out_count = 0;
    if (!eh || eh->e_shoff == 0 || eh->e_shnum == 0) return 1;

    if (fseek(f, (long)eh->e_shoff, SEEK_SET) != 0) return 0;
    Elf64_Shdr *shdrs = (Elf64_Shdr *)calloc(eh->e_shnum, sizeof(Elf64_Shdr));
    if (!shdrs) return 0;
    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        if (!read_exact(f, &shdrs[i], sizeof(Elf64_Shdr))) { free(shdrs); return 0; }
    }

    for (uint16_t i = 0; i < eh->e_shnum; i++) {
        if (shdrs[i].sh_type != SHT_SYMTAB && shdrs[i].sh_type != SHT_DYNSYM) continue;
        if (shdrs[i].sh_entsize == 0) continue;
        uint32_t str_idx = shdrs[i].sh_link;
        if (str_idx >= eh->e_shnum) continue;

        Elf64_Shdr *strtab = &shdrs[str_idx];
        if (strtab->sh_type != SHT_STRTAB || strtab->sh_size == 0) continue;

        char *strs = (char *)malloc((size_t)strtab->sh_size);
        if (!strs) { free(shdrs); return 0; }
        if (fseek(f, (long)strtab->sh_offset, SEEK_SET) != 0) { free(strs); free(shdrs); return 0; }
        if (!read_exact(f, strs, (size_t)strtab->sh_size)) { free(strs); free(shdrs); return 0; }

        size_t sym_count = (size_t)(shdrs[i].sh_size / shdrs[i].sh_entsize);
        if (fseek(f, (long)shdrs[i].sh_offset, SEEK_SET) != 0) { free(strs); free(shdrs); return 0; }

        for (size_t s = 0; s < sym_count; s++) {
            Elf64_Sym sym = {0};
            if (!read_exact(f, &sym, sizeof(sym))) { free(strs); free(shdrs); return 0; }
            if (sym.st_name == 0 || sym.st_name >= strtab->sh_size) continue;
            const char *name = strs + sym.st_name;
            if (*name == '\0') continue;
            Symbol *mem = (Symbol *)realloc(*out_syms, (*out_count + 1) * sizeof(Symbol));
            if (!mem) { free(strs); free(shdrs); symbols_free(*out_syms, *out_count); return 0; }
            *out_syms = mem;
            (*out_syms)[*out_count].name = dup_string(name);
            (*out_syms)[*out_count].value = sym.st_value;
            (*out_syms)[*out_count].size = sym.st_size;
            (*out_syms)[*out_count].info = sym.st_info;
            (*out_count)++;
        }

        free(strs);
    }

    free(shdrs);
    return 1;
}

static void stats_add_mnemonic(MnemonicCount **items, size_t *count, size_t *cap, const char *mn) {
    if (!mn || !*mn) return;
    for (size_t i = 0; i < *count; i++) {
        if (strcmp((*items)[i].name, mn) == 0) { (*items)[i].count++; return; }
    }
    if (*count + 1 > *cap) {
        size_t next = (*cap == 0) ? 8 : (*cap * 2);
        MnemonicCount *mem = (MnemonicCount *)realloc(*items, next * sizeof(MnemonicCount));
        if (!mem) return;
        *items = mem;
        *cap = next;
    }
    (*items)[*count].name = mn;
    (*items)[*count].count = 1;
    (*count)++;
}

static void decode_inst(uint32_t inst, uint64_t pc, Decoded *out) {
    memset(out, 0, sizeof(*out));
    out->opcode = inst & 0x7f;
    out->rd = (inst >> 7) & 0x1f;
    out->funct3 = (inst >> 12) & 0x7;
    out->rs1 = (inst >> 15) & 0x1f;
    out->rs2 = (inst >> 20) & 0x1f;
    out->funct7 = (inst >> 25) & 0x7f;

    switch (out->opcode) {
        case 0x33: {
            if (out->funct7 == 0x00 && out->funct3 == 0x00) { out->mnemonic = "add"; fmt_r(out->text, sizeof(out->text), "add", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x20 && out->funct3 == 0x00) { out->mnemonic = "sub"; fmt_r(out->text, sizeof(out->text), "sub", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x00) { out->mnemonic = "mul"; fmt_r(out->text, sizeof(out->text), "mul", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x01) { out->mnemonic = "mulh"; fmt_r(out->text, sizeof(out->text), "mulh", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x02) { out->mnemonic = "mulhsu"; fmt_r(out->text, sizeof(out->text), "mulhsu", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x03) { out->mnemonic = "mulhu"; fmt_r(out->text, sizeof(out->text), "mulhu", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x04) { out->mnemonic = "div"; fmt_r(out->text, sizeof(out->text), "div", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x05) { out->mnemonic = "divu"; fmt_r(out->text, sizeof(out->text), "divu", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x06) { out->mnemonic = "rem"; fmt_r(out->text, sizeof(out->text), "rem", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x01 && out->funct3 == 0x07) { out->mnemonic = "remu"; fmt_r(out->text, sizeof(out->text), "remu", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x01) { out->mnemonic = "sll"; fmt_r(out->text, sizeof(out->text), "sll", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x05) { out->mnemonic = "srl"; fmt_r(out->text, sizeof(out->text), "srl", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x20 && out->funct3 == 0x05) { out->mnemonic = "sra"; fmt_r(out->text, sizeof(out->text), "sra", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x07) { out->mnemonic = "and"; fmt_r(out->text, sizeof(out->text), "and", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x06) { out->mnemonic = "or"; fmt_r(out->text, sizeof(out->text), "or", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x04) { out->mnemonic = "xor"; fmt_r(out->text, sizeof(out->text), "xor", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x02) { out->mnemonic = "slt"; fmt_r(out->text, sizeof(out->text), "slt", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x00 && out->funct3 == 0x03) { out->mnemonic = "sltu"; fmt_r(out->text, sizeof(out->text), "sltu", out->rd, out->rs1, out->rs2); return; }
            break;
        }
        case 0x13: {
            out->imm = sign_extend(inst >> 20, 12);
            out->has_imm = 1;
            if (out->funct3 == 0x00) { out->mnemonic = "addi"; fmt_i(out->text, sizeof(out->text), "addi", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x07) { out->mnemonic = "andi"; fmt_i(out->text, sizeof(out->text), "andi", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x06) { out->mnemonic = "ori"; fmt_i(out->text, sizeof(out->text), "ori", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x04) { out->mnemonic = "xori"; fmt_i(out->text, sizeof(out->text), "xori", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x02) { out->mnemonic = "slti"; fmt_i(out->text, sizeof(out->text), "slti", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x03) { out->mnemonic = "sltiu"; fmt_i(out->text, sizeof(out->text), "sltiu", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x01 && out->funct7 == 0x00) { out->mnemonic = "slli"; out->imm = (inst >> 20) & 0x3f; fmt_i(out->text, sizeof(out->text), "slli", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x05 && out->funct7 == 0x00) { out->mnemonic = "srli"; out->imm = (inst >> 20) & 0x3f; fmt_i(out->text, sizeof(out->text), "srli", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x05 && out->funct7 == 0x20) { out->mnemonic = "srai"; out->imm = (inst >> 20) & 0x3f; fmt_i(out->text, sizeof(out->text), "srai", out->rd, out->rs1, out->imm); return; }
            break;
        }
        case 0x03: {
            out->imm = sign_extend(inst >> 20, 12);
            out->has_imm = 1;
            if (out->funct3 == 0x00) { out->mnemonic = "ldb"; fmt_load(out->text, sizeof(out->text), "ldb", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x04) { out->mnemonic = "ldbu"; fmt_load(out->text, sizeof(out->text), "ldbu", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x01) { out->mnemonic = "ldh"; fmt_load(out->text, sizeof(out->text), "ldh", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x05) { out->mnemonic = "ldhu"; fmt_load(out->text, sizeof(out->text), "ldhu", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x02) { out->mnemonic = "ldw"; fmt_load(out->text, sizeof(out->text), "ldw", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x06) { out->mnemonic = "ldwu"; fmt_load(out->text, sizeof(out->text), "ldwu", out->rd, out->rs1, out->imm); return; }
            if (out->funct3 == 0x03) { out->mnemonic = "ld"; fmt_load(out->text, sizeof(out->text), "ld", out->rd, out->rs1, out->imm); return; }
            break;
        }
        case 0x23: {
            uint32_t imm12 = ((inst >> 25) << 5) | ((inst >> 7) & 0x1f);
            out->imm = sign_extend(imm12, 12);
            out->has_imm = 1;
            if (out->funct3 == 0x00) { out->mnemonic = "stb"; fmt_store(out->text, sizeof(out->text), "stb", out->rs2, out->rs1, out->imm); return; }
            if (out->funct3 == 0x01) { out->mnemonic = "sth"; fmt_store(out->text, sizeof(out->text), "sth", out->rs2, out->rs1, out->imm); return; }
            if (out->funct3 == 0x02) { out->mnemonic = "stw"; fmt_store(out->text, sizeof(out->text), "stw", out->rs2, out->rs1, out->imm); return; }
            if (out->funct3 == 0x03) { out->mnemonic = "st"; fmt_store(out->text, sizeof(out->text), "st", out->rs2, out->rs1, out->imm); return; }
            break;
        }
        case 0x63: {
            uint32_t imm = ((inst >> 31) & 0x1) << 12;
            imm |= ((inst >> 7) & 0x1) << 11;
            imm |= ((inst >> 25) & 0x3f) << 5;
            imm |= ((inst >> 8) & 0x0f) << 1;
            out->imm = sign_extend(imm, 13);
            out->has_imm = 1;
            out->is_branch = 1;
            out->is_conditional = 1;
            out->has_target = 1;
            out->target = pc + (int64_t)out->imm;
            if (out->funct3 == 0x00) { out->mnemonic = "beq"; fmt_branch(out->text, sizeof(out->text), "beq", out->rs1, out->rs2, out->imm, pc); return; }
            if (out->funct3 == 0x01) { out->mnemonic = "bne"; fmt_branch(out->text, sizeof(out->text), "bne", out->rs1, out->rs2, out->imm, pc); return; }
            if (out->funct3 == 0x04) { out->mnemonic = "blt"; fmt_branch(out->text, sizeof(out->text), "blt", out->rs1, out->rs2, out->imm, pc); return; }
            if (out->funct3 == 0x05) { out->mnemonic = "bge"; fmt_branch(out->text, sizeof(out->text), "bge", out->rs1, out->rs2, out->imm, pc); return; }
            if (out->funct3 == 0x06) { out->mnemonic = "bltu"; fmt_branch(out->text, sizeof(out->text), "bltu", out->rs1, out->rs2, out->imm, pc); return; }
            if (out->funct3 == 0x07) { out->mnemonic = "bgeu"; fmt_branch(out->text, sizeof(out->text), "bgeu", out->rs1, out->rs2, out->imm, pc); return; }
            break;
        }
        case 0x6f: {
            uint32_t imm = ((inst >> 31) & 0x1) << 20;
            imm |= ((inst >> 12) & 0xff) << 12;
            imm |= ((inst >> 20) & 0x1) << 11;
            imm |= ((inst >> 21) & 0x3ff) << 1;
            out->imm = sign_extend(imm, 21);
            out->has_imm = 1;
            out->is_jump = 1;
            out->has_target = 1;
            out->target = pc + (int64_t)out->imm;
            out->mnemonic = "jal";
            snprintf(out->text, sizeof(out->text), "jal %s, 0x%llx", reg_name(out->rd), (unsigned long long)out->target);
            return;
        }
        case 0x67: {
            out->imm = sign_extend(inst >> 20, 12);
            out->has_imm = 1;
            out->is_jump = 1;
            if (out->funct3 == 0x00) {
                out->mnemonic = "jalr";
                snprintf(out->text, sizeof(out->text), "jalr %s, %d(%s)", reg_name(out->rd), out->imm, reg_name(out->rs1));
                return;
            }
            break;
        }
        case 0x37: {
            out->imm = (int32_t)(inst & 0xfffff000u);
            out->has_imm = 1;
            out->mnemonic = "movhi";
            snprintf(out->text, sizeof(out->text), "movhi %s, 0x%x", reg_name(out->rd), (unsigned int)out->imm);
            return;
        }
        case 0x17: {
            out->imm = (int32_t)(inst & 0xfffff000u);
            out->has_imm = 1;
            out->mnemonic = "movpc";
            snprintf(out->text, sizeof(out->text), "movpc %s, 0x%x", reg_name(out->rd), (unsigned int)out->imm);
            return;
        }
        case 0x73: {
            out->imm = sign_extend(inst >> 20, 12);
            out->has_imm = 1;
            if (out->funct3 == 0x00 && out->rd == 0 && out->rs1 == 0) {
                if (out->imm == 0) { out->mnemonic = "ecall"; snprintf(out->text, sizeof(out->text), "ecall"); return; }
                if (out->imm == 1) { out->mnemonic = "ebreak"; snprintf(out->text, sizeof(out->text), "ebreak"); return; }
            }
            if (out->funct3 == 0x01) { out->mnemonic = "csrrw"; snprintf(out->text, sizeof(out->text), "csrrw %s, 0x%x, %s", reg_name(out->rd), (unsigned)(out->imm & 0xfff), reg_name(out->rs1)); return; }
            if (out->funct3 == 0x02) { out->mnemonic = "csrrs"; snprintf(out->text, sizeof(out->text), "csrrs %s, 0x%x, %s", reg_name(out->rd), (unsigned)(out->imm & 0xfff), reg_name(out->rs1)); return; }
            if (out->funct3 == 0x03) { out->mnemonic = "csrrc"; snprintf(out->text, sizeof(out->text), "csrrc %s, 0x%x, %s", reg_name(out->rd), (unsigned)(out->imm & 0xfff), reg_name(out->rs1)); return; }
            if (out->funct3 == 0x05) { out->mnemonic = "csrrwi"; snprintf(out->text, sizeof(out->text), "csrrwi %s, 0x%x, %u", reg_name(out->rd), (unsigned)(out->imm & 0xfff), out->rs1); return; }
            if (out->funct3 == 0x06) { out->mnemonic = "csrrsi"; snprintf(out->text, sizeof(out->text), "csrrsi %s, 0x%x, %u", reg_name(out->rd), (unsigned)(out->imm & 0xfff), out->rs1); return; }
            if (out->funct3 == 0x07) { out->mnemonic = "csrrci"; snprintf(out->text, sizeof(out->text), "csrrci %s, 0x%x, %u", reg_name(out->rd), (unsigned)(out->imm & 0xfff), out->rs1); return; }
            break;
        }
        case 0x0f: {
            if (out->funct3 == 0x00) { out->mnemonic = "fence"; snprintf(out->text, sizeof(out->text), "fence"); return; }
            break;
        }
        case 0x2f: {
            if (out->funct7 == 0x04 && out->funct3 == 0x02) { out->mnemonic = "amoswap.w"; fmt_r(out->text, sizeof(out->text), "amoswap.w", out->rd, out->rs1, out->rs2); return; }
            if (out->funct7 == 0x04 && out->funct3 == 0x03) { out->mnemonic = "amoswap.d"; fmt_r(out->text, sizeof(out->text), "amoswap.d", out->rd, out->rs1, out->rs2); return; }
            break;
        }
        case 0x2b: {
            out->mnemonic = "cap";
            snprintf(out->text, sizeof(out->text), "cap (opcode 0x2b)");
            return;
        }
        default:
            break;
    }

    snprintf(out->text, sizeof(out->text), ".word 0x%08x", inst);
}

static void print_strings(const uint8_t *buf, size_t size, uint64_t base) {
    size_t i = 0;
    while (i < size) {
        if (isprint(buf[i])) {
            size_t start = i;
            while (i < size && isprint(buf[i])) i++;
            size_t len = i - start;
            if (len >= 4) {
                printf("String @ 0x%08llx: \"", (unsigned long long)(base + start));
                for (size_t j = start; j < start + len; j++) {
                    unsigned char c = buf[j];
                    if (c == '\\' || c == '"') {
                        putchar('\\');
                        putchar(c);
                    } else {
                        putchar(c);
                    }
                }
                printf("\"\n");
            }
        } else {
            i++;
        }
    }
}

static void hexdump(const uint8_t *buf, size_t size, uint64_t base) {
    for (size_t i = 0; i < size; i += 16) {
        printf("0x%08llx  ", (unsigned long long)(base + i));
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) printf("%02x ", buf[i + j]);
            else printf("   ");
        }
        printf(" | ");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = buf[i + j];
            putchar(isprint(c) ? c : '.');
        }
        printf("\n");
    }
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t av = *(const uint64_t *)a;
    uint64_t bv = *(const uint64_t *)b;
    if (av < bv) return -1;
    if (av > bv) return 1;
    return 0;
}

static int read_exact(FILE *f, void *buf, size_t len) {
    return fread(buf, 1, len, f) == len;
}

int main(int argc, char **argv) {
    Options opt;
    if (!parse_options(argc, argv, &opt)) {
        usage();
        return 1;
    }

    FILE *f = fopen(opt.path, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    Elf64_Ehdr eh = {0};
    if (!read_exact(f, &eh, sizeof(eh))) {
        fprintf(stderr, "error: failed to read ELF header\n");
        fclose(f);
        return 1;
    }

    if (eh.e_ident[0] != ELF_MAGIC0 || eh.e_ident[1] != ELF_MAGIC1 || eh.e_ident[2] != ELF_MAGIC2 || eh.e_ident[3] != ELF_MAGIC3) {
        fprintf(stderr, "error: not an ELF file\n");
        fclose(f);
        return 1;
    }
    if (eh.e_ident[4] != ELFCLASS64 || eh.e_ident[5] != ELFDATA2LSB || eh.e_ident[6] != EV_CURRENT) {
        fprintf(stderr, "error: unsupported ELF class/data\n");
        fclose(f);
        return 1;
    }

    if (!opt.json) {
        printf("ELF64 entry: 0x%llx\n", (unsigned long long)eh.e_entry);
        printf("Program headers: %u @ 0x%llx\n", (unsigned)eh.e_phnum, (unsigned long long)eh.e_phoff);
    }

    if (eh.e_phoff == 0 || eh.e_phnum == 0) {
        fprintf(stderr, "error: no program headers\n");
        fclose(f);
        return 1;
    }

    if (fseek(f, (long)eh.e_phoff, SEEK_SET) != 0) {
        fprintf(stderr, "error: failed to seek program headers\n");
        fclose(f);
        return 1;
    }

    Elf64_Phdr *phdrs = (Elf64_Phdr *)calloc(eh.e_phnum, sizeof(Elf64_Phdr));
    if (!phdrs) {
        fprintf(stderr, "error: out of memory\n");
        fclose(f);
        return 1;
    }

    for (uint16_t i = 0; i < eh.e_phnum; i++) {
        if (!read_exact(f, &phdrs[i], sizeof(Elf64_Phdr))) {
            fprintf(stderr, "error: failed to read program header\n");
            free(phdrs);
            fclose(f);
            return 1;
        }
    }

    Symbol *symbols = NULL;
    size_t symbol_count = 0;
    if (!read_symbols(f, &eh, &symbols, &symbol_count)) {
        fprintf(stderr, "error: failed to read symbols\n");
        free(phdrs);
        fclose(f);
        return 1;
    }

    if (!opt.json) {
        for (uint16_t i = 0; i < eh.e_phnum; i++) {
            Elf64_Phdr *ph = &phdrs[i];
            if (ph->p_type != PT_LOAD) continue;
            printf("PH[%u]: vaddr=0x%llx off=0x%llx filesz=0x%llx memsz=0x%llx flags=%c%c%c\n",
                   (unsigned)i,
                   (unsigned long long)ph->p_vaddr,
                   (unsigned long long)ph->p_offset,
                   (unsigned long long)ph->p_filesz,
                   (unsigned long long)ph->p_memsz,
                   (ph->p_flags & PF_R) ? 'R' : '-',
                   (ph->p_flags & PF_W) ? 'W' : '-',
                   (ph->p_flags & PF_X) ? 'X' : '-');
        }
    }

    uint64_t opcode_counts[128] = {0};
    MnemonicCount *mn_stats = NULL;
    size_t mn_count = 0;
    size_t mn_cap = 0;

    if (opt.json) {
        printf("{\n");
        printf("  \"entry\": %llu,\n", (unsigned long long)eh.e_entry);
        printf("  \"programHeaders\": [\n");
        int first_ph = 1;
        for (uint16_t i = 0; i < eh.e_phnum; i++) {
            Elf64_Phdr *ph = &phdrs[i];
            if (ph->p_type != PT_LOAD) continue;
            if (!first_ph) printf(",\n");
            first_ph = 0;
            printf("    {\"index\":%u,\"vaddr\":%llu,\"offset\":%llu,\"filesz\":%llu,\"memsz\":%llu,\"flags\":\"%c%c%c\"}",
                   (unsigned)i,
                   (unsigned long long)ph->p_vaddr,
                   (unsigned long long)ph->p_offset,
                   (unsigned long long)ph->p_filesz,
                   (unsigned long long)ph->p_memsz,
                   (ph->p_flags & PF_R) ? 'R' : '-',
                   (ph->p_flags & PF_W) ? 'W' : '-',
                   (ph->p_flags & PF_X) ? 'X' : '-');
        }
        printf("\n  ],\n");

        printf("  \"symbols\": [\n");
        for (size_t s = 0; s < symbol_count; s++) {
            if (s) printf(",\n");
            printf("    {\"name\":\"");
            json_escape(stdout, symbols[s].name);
            printf("\",\"value\":%llu,\"size\":%llu,\"info\":%u}",
                   (unsigned long long)symbols[s].value,
                   (unsigned long long)symbols[s].size,
                   (unsigned)symbols[s].info);
        }
        printf("\n  ],\n");
        printf("  \"segments\": [\n");
    }

    int first_seg = 1;
    for (uint16_t i = 0; i < eh.e_phnum; i++) {
        Elf64_Phdr *ph = &phdrs[i];
        if (ph->p_type != PT_LOAD) continue;
        if (ph->p_filesz == 0) continue;

        uint8_t *buf = (uint8_t *)malloc((size_t)ph->p_filesz);
        if (!buf) {
            fprintf(stderr, "error: out of memory\n");
            free(phdrs);
            fclose(f);
            return 1;
        }

        if (fseek(f, (long)ph->p_offset, SEEK_SET) != 0) {
            fprintf(stderr, "error: failed to seek segment\n");
            free(buf);
            free(phdrs);
            fclose(f);
            return 1;
        }
        if (!read_exact(f, buf, (size_t)ph->p_filesz)) {
            fprintf(stderr, "error: failed to read segment\n");
            free(buf);
            free(phdrs);
            fclose(f);
            return 1;
        }

        Label *labels = NULL;
        size_t label_count = 0;
        size_t label_cap = 0;
        AddrList blocks = {0};
        Edge *edges = NULL;
        size_t edge_count = 0;
        size_t edge_cap = 0;
        uint64_t disasm_end = ph->p_vaddr + ph->p_filesz;

        if (opt.json) {
            if (!first_seg) printf(",\n");
            first_seg = 0;
            printf("    {\"index\":%u,\"vaddr\":%llu,\"offset\":%llu,\"filesz\":%llu,\"memsz\":%llu,\"flags\":\"%c%c%c\",",
                   (unsigned)i,
                   (unsigned long long)ph->p_vaddr,
                   (unsigned long long)ph->p_offset,
                   (unsigned long long)ph->p_filesz,
                   (unsigned long long)ph->p_memsz,
                   (ph->p_flags & PF_R) ? 'R' : '-',
                   (ph->p_flags & PF_W) ? 'W' : '-',
                   (ph->p_flags & PF_X) ? 'X' : '-');
            printf("\"disassembly\":[");
        }

        int first_insn = 1;
        if (ph->p_flags & PF_X) {
            label_add(&labels, &label_count, &label_cap, ph->p_vaddr);
            addrlist_add_unique(&blocks, ph->p_vaddr);

            for (uint64_t off = 0; off + 4 <= ph->p_filesz; off += 4) {
                uint32_t inst = (uint32_t)buf[off]
                    | ((uint32_t)buf[off + 1] << 8)
                    | ((uint32_t)buf[off + 2] << 16)
                    | ((uint32_t)buf[off + 3] << 24);
                uint64_t pc = ph->p_vaddr + off;
                Decoded dec;
                decode_inst(inst, pc, &dec);
                opcode_counts[dec.opcode & 0x7f]++;
                stats_add_mnemonic(&mn_stats, &mn_count, &mn_cap, dec.mnemonic ? dec.mnemonic : "");
                if (dec.has_target) {
                    uint64_t target = dec.target;
                    if (target >= ph->p_vaddr && target < ph->p_vaddr + ph->p_filesz) {
                        label_add(&labels, &label_count, &label_cap, target);
                        addrlist_add_unique(&blocks, target);
                        edge_add(&edges, &edge_count, &edge_cap, pc, target, 0);
                    }
                }
                if (dec.is_branch && dec.is_conditional) {
                    uint64_t fallthrough = pc + 4;
                    if (fallthrough < ph->p_vaddr + ph->p_filesz) {
                        addrlist_add_unique(&blocks, fallthrough);
                        edge_add(&edges, &edge_count, &edge_cap, pc, fallthrough, 1);
                    }
                }
                if (opt.stop_at_ebreak && dec.mnemonic && strcmp(dec.mnemonic, "ebreak") == 0) {
                    disasm_end = pc + 4;
                    break;
                }
            }

            if (!opt.json) {
                printf("\nDisassembly (segment %u, vaddr=0x%llx):\n", (unsigned)i, (unsigned long long)ph->p_vaddr);
            }
            for (uint64_t off = 0; off + 4 <= ph->p_filesz && ph->p_vaddr + off < disasm_end; off += 4) {
                uint32_t inst = (uint32_t)buf[off]
                    | ((uint32_t)buf[off + 1] << 8)
                    | ((uint32_t)buf[off + 2] << 16)
                    | ((uint32_t)buf[off + 3] << 24);
                uint64_t pc = ph->p_vaddr + off;
                Decoded dec;
                decode_inst(inst, pc, &dec);
                int lbl = label_find(labels, label_count, pc);
                char fields[160];
                if (dec.has_imm) {
                    snprintf(fields, sizeof(fields), "op=0x%02x f3=0x%x f7=0x%02x rd=%u rs1=%u rs2=%u imm=%d",
                             dec.opcode, dec.funct3, dec.funct7, dec.rd, dec.rs1, dec.rs2, dec.imm);
                } else {
                    snprintf(fields, sizeof(fields), "op=0x%02x f3=0x%x f7=0x%02x rd=%u rs1=%u rs2=%u imm=-",
                             dec.opcode, dec.funct3, dec.funct7, dec.rd, dec.rs1, dec.rs2);
                }

                int tgt_label = (dec.has_target) ? label_find(labels, label_count, dec.target) : -1;
                if (!opt.json) {
                    if (lbl >= 0) printf("L%d:\n", lbl);
                    printf("0x%08llx: 0x%08x  %-24s  [%s]", (unsigned long long)pc, inst, dec.text, fields);
                    if (tgt_label >= 0) printf("  ; L%d", tgt_label);
                    printf("\n");
                } else {
                    if (!first_insn) printf(",");
                    first_insn = 0;
                    printf("{\"addr\":%llu,\"inst\":%u,\"text\":\"", (unsigned long long)pc, inst);
                    json_escape(stdout, dec.text);
                    printf("\",\"fields\":\"");
                    json_escape(stdout, fields);
                    printf("\"");
                    if (lbl >= 0) printf(",\"label\":%d", lbl);
                    if (tgt_label >= 0) printf(",\"targetLabel\":%d", tgt_label);
                    if (dec.has_target) printf(",\"target\":%llu", (unsigned long long)dec.target);
                    printf("}");
                }

                if (opt.stop_at_ebreak && dec.mnemonic && strcmp(dec.mnemonic, "ebreak") == 0) { break; }
            }
        }

        if (opt.json) {
            printf("],");
        }

        if (!(opt.only_text && (ph->p_flags & PF_X))) {
            if (!opt.json) {
                printf("\nStrings (segment %u, vaddr=0x%llx):\n", (unsigned)i, (unsigned long long)ph->p_vaddr);
                print_strings(buf, (size_t)ph->p_filesz, ph->p_vaddr);

                printf("\nHexdump (segment %u, vaddr=0x%llx):\n", (unsigned)i, (unsigned long long)ph->p_vaddr);
                hexdump(buf, (size_t)ph->p_filesz, ph->p_vaddr);
            } else {
                printf("\"strings\":[");
                size_t first = 1;
                size_t idx = 0;
                while (idx < ph->p_filesz) {
                    if (isprint(buf[idx])) {
                        size_t start = idx;
                        while (idx < ph->p_filesz && isprint(buf[idx])) idx++;
                        size_t len = idx - start;
                        if (len >= 4) {
                            if (!first) printf(",");
                            first = 0;
                            printf("{\"addr\":%llu,\"text\":\"", (unsigned long long)(ph->p_vaddr + start));
                            for (size_t j = start; j < start + len; j++) {
                                unsigned char c = buf[j];
                                if (c == '\\' || c == '"') { fputc('\\', stdout); fputc(c, stdout); }
                                else if (c == '\n') fputs("\\n", stdout);
                                else if (c == '\r') fputs("\\r", stdout);
                                else if (c == '\t') fputs("\\t", stdout);
                                else fputc(c, stdout);
                            }
                            printf("\"}");
                        }
                    } else {
                        idx++;
                    }
                }
                printf("],");

                printf("\"hexdump\":[");
                for (size_t off = 0; off < ph->p_filesz; off += 16) {
                    if (off) printf(",");
                    printf("{\"addr\":%llu,\"bytes\":\"", (unsigned long long)(ph->p_vaddr + off));
                    for (size_t j = 0; j < 16 && off + j < ph->p_filesz; j++) {
                        printf("%02x", buf[off + j]);
                    }
                    printf("\"}");
                }
                printf("],");
            }
        } else if (opt.json) {
            printf("\"strings\":[],\"hexdump\":[],");
        }

        if (ph->p_flags & PF_X) {
            if (blocks.count > 1) {
                qsort(blocks.items, blocks.count, sizeof(uint64_t), cmp_u64);
            }
            if (!opt.json) {
                printf("\nCFG Summary (segment %u):\n", (unsigned)i);
                printf("Basic blocks: %zu\n", blocks.count);
            }

            if (opt.json) printf("\"blocks\":[");
            for (size_t b = 0; b < blocks.count; b++) {
                uint64_t start = blocks.items[b];
                uint64_t end = (b + 1 < blocks.count) ? blocks.items[b + 1] : disasm_end;
                if (end > disasm_end) end = disasm_end;
                size_t count = 0;
                if (end > start) count = (size_t)((end - start) / 4);
                int lbl = label_find(labels, label_count, start);
                if (!opt.json) {
                    if (lbl >= 0) printf("  L%d @ 0x%08llx (insts=%zu)\n", lbl, (unsigned long long)start, count);
                    else printf("  0x%08llx (insts=%zu)\n", (unsigned long long)start, count);
                } else {
                    if (b) printf(",");
                    printf("{\"addr\":%llu,\"count\":%zu", (unsigned long long)start, count);
                    if (lbl >= 0) printf(",\"label\":%d", lbl);
                    printf("}");
                }
            }
            if (opt.json) printf("],");

            if (!opt.json && edge_count > 0) printf("Edges:\n");
            if (opt.json) printf("\"edges\":[");
            for (size_t e = 0; e < edge_count; e++) {
                int from_lbl = label_find(labels, label_count, edges[e].from);
                int to_lbl = label_find(labels, label_count, edges[e].to);
                if (!opt.json) {
                    if (from_lbl >= 0) printf("  L%d -> ", from_lbl);
                    else printf("  0x%08llx -> ", (unsigned long long)edges[e].from);
                    if (to_lbl >= 0) printf("L%d", to_lbl);
                    else printf("0x%08llx", (unsigned long long)edges[e].to);
                    printf(edges[e].fallthrough ? " (fallthrough)\n" : "\n");
                } else {
                    if (e) printf(",");
                    printf("{\"from\":%llu,\"to\":%llu,\"fallthrough\":%s", (unsigned long long)edges[e].from, (unsigned long long)edges[e].to, edges[e].fallthrough ? "true" : "false");
                    if (from_lbl >= 0) printf(",\"fromLabel\":%d", from_lbl);
                    if (to_lbl >= 0) printf(",\"toLabel\":%d", to_lbl);
                    printf("}");
                }
            }
            if (opt.json) printf("]");
        }

        if (opt.json) printf("}");

        free(labels);
        free(blocks.items);
        free(edges);

        free(buf);
    }

    free(phdrs);
    if (!opt.json) {
        if (symbol_count > 0) {
            printf("\nSymbols:\n");
            for (size_t s = 0; s < symbol_count; s++) {
                printf("  %s @ 0x%llx (size=%llu)\n", symbols[s].name, (unsigned long long)symbols[s].value, (unsigned long long)symbols[s].size);
            }
        } else {
            printf("\nSymbols: <none>\n");
        }
    }
    if (opt.json) {
        printf("\n  ]");
        if (opt.stats) {
            printf(",\n  \"stats\":{\"mnemonics\":[");
            for (size_t i = 0; i < mn_count; i++) {
                if (i) printf(",");
                printf("{\"name\":\"");
                json_escape(stdout, mn_stats[i].name);
                printf("\",\"count\":%d}", mn_stats[i].count);
            }
            printf("],\"opcodes\":[");
            int first = 1;
            for (int i = 0; i < 128; i++) {
                if (opcode_counts[i] == 0) continue;
                if (!first) printf(",");
                first = 0;
                printf("{\"opcode\":%d,\"count\":%llu}", i, (unsigned long long)opcode_counts[i]);
            }
            printf("]}\n");
        } else {
            printf("\n");
        }
        printf("}\n");
    }
    symbols_free(symbols, symbol_count);
    free(mn_stats);
    fclose(f);
    return 0;
}
