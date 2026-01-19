#include "asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SRC_FILE,
    SRC_LINES
} SourceKind;

typedef struct {
    SourceKind kind;
    FILE *file;
    bool own_file;
    char **lines;
    size_t count;
    size_t index;
    int line_no;
    char dir[512];
} LineSource;

typedef struct {
    LineSource *items;
    size_t count;
    size_t cap;
} LineStack;

typedef struct {
    char **tokens;
    int count;
} MacroLine;

typedef struct {
    char *name;
    char **params;
    int param_count;
    MacroLine *lines;
    size_t line_count;
    size_t line_cap;
} Macro;

typedef struct {
    Macro *items;
    size_t count;
    size_t cap;
} MacroTable;

static void stack_free(LineStack *stack) {
    for (size_t i = 0; i < stack->count; i++) {
        LineSource *src = &stack->items[i];
        if (src->kind == SRC_FILE && src->file && src->own_file) fclose(src->file);
        if (src->kind == SRC_LINES && src->lines) {
            for (size_t l = 0; l < src->count; l++) free(src->lines[l]);
            free(src->lines);
        }
    }
    free(stack->items);
    stack->items = NULL;
    stack->count = 0;
    stack->cap = 0;
}

static int stack_push(LineStack *stack, LineSource src) {
    if (stack->count + 1 > stack->cap) {
        size_t next = (stack->cap == 0) ? 4 : stack->cap * 2;
        LineSource *mem = (LineSource *)realloc(stack->items, next * sizeof(LineSource));
        if (!mem) return 0;
        stack->items = mem;
        stack->cap = next;
    }
    stack->items[stack->count++] = src;
    return 1;
}

static void stack_pop(LineStack *stack) {
    if (stack->count == 0) return;
    LineSource *src = &stack->items[stack->count - 1];
    if (src->kind == SRC_FILE && src->file && src->own_file) fclose(src->file);
    if (src->kind == SRC_LINES && src->lines) {
        for (size_t l = 0; l < src->count; l++) free(src->lines[l]);
        free(src->lines);
    }
    stack->count--;
}

static int stack_next_line(LineStack *stack, char *out, size_t out_cap, LineSource **out_src) {
    while (stack->count > 0) {
        LineSource *src = &stack->items[stack->count - 1];
        if (src->kind == SRC_FILE) {
            if (!fgets(out, (int)out_cap, src->file)) {
                stack_pop(stack);
                continue;
            }
            src->line_no++;
            if (out_src) *out_src = src;
            return 1;
        }
        if (src->kind == SRC_LINES) {
            if (src->index >= src->count) {
                stack_pop(stack);
                continue;
            }
            strncpy(out, src->lines[src->index++], out_cap - 1);
            out[out_cap - 1] = '\0';
            src->line_no++;
            if (out_src) *out_src = src;
            return 1;
        }
        stack_pop(stack);
    }
    return 0;
}

static void macro_table_free(MacroTable *table) {
    for (size_t i = 0; i < table->count; i++) {
        Macro *m = &table->items[i];
        free(m->name);
        for (int p = 0; p < m->param_count; p++) free(m->params[p]);
        free(m->params);
        for (size_t l = 0; l < m->line_count; l++) {
            for (int t = 0; t < m->lines[l].count; t++) free(m->lines[l].tokens[t]);
            free(m->lines[l].tokens);
        }
        free(m->lines);
    }
    free(table->items);
    table->items = NULL;
    table->count = 0;
    table->cap = 0;
}

static Macro *macro_find(MacroTable *table, const char *name) {
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].name, name) == 0) return &table->items[i];
    }
    return NULL;
}

static int macro_add(MacroTable *table, Macro macro) {
    if (table->count + 1 > table->cap) {
        size_t next = (table->cap == 0) ? 4 : table->cap * 2;
        Macro *mem = (Macro *)realloc(table->items, next * sizeof(Macro));
        if (!mem) return 0;
        table->items = mem;
        table->cap = next;
    }
    table->items[table->count++] = macro;
    return 1;
}

static void get_dirname(const char *path, char *out, size_t out_cap) {
    if (!path || !*path) { strncpy(out, ".", out_cap); out[out_cap - 1] = '\0'; return; }
    const char *slash = strrchr(path, '/');
    if (!slash) { strncpy(out, ".", out_cap); out[out_cap - 1] = '\0'; return; }
    size_t len = (size_t)(slash - path);
    if (len >= out_cap) len = out_cap - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

static int strip_quotes(const char *in, char *out, size_t out_cap) {
    if (!in || !*in) return 0;
    size_t len = strlen(in);
    if (in[0] == '"' && len >= 2 && in[len - 1] == '"') {
        size_t n = len - 2;
        if (n >= out_cap) n = out_cap - 1;
        memcpy(out, in + 1, n);
        out[n] = '\0';
        return 1;
    }
    strncpy(out, in, out_cap - 1);
    out[out_cap - 1] = '\0';
    return 1;
}

static int resolve_include_path(const char *dir, const char *token, char *out, size_t out_cap) {
    char tmp[512];
    if (!strip_quotes(token, tmp, sizeof(tmp))) return 0;
    if (tmp[0] == '/') {
        strncpy(out, tmp, out_cap - 1);
        out[out_cap - 1] = '\0';
        return 1;
    }
    if (!dir || !*dir) dir = ".";
    snprintf(out, out_cap, "%s/%s", dir, tmp);
    return 1;
}

static int macro_capture(LineStack *stack, MacroTable *macros, char *tokens[], int count) {
    if (count < 2) return 0;
    Macro macro = {0};
    macro.name = strdup(tokens[1]);
    if (!macro.name) return 0;
    if (count > 2) {
        macro.param_count = count - 2;
        macro.params = (char **)calloc((size_t)macro.param_count, sizeof(char *));
        if (!macro.params) return 0;
        for (int i = 0; i < macro.param_count; i++) {
            macro.params[i] = strdup(tokens[i + 2]);
            if (!macro.params[i]) return 0;
        }
    }

    char line[MAX_LINE];
    LineSource *cur = NULL;
    while (stack_next_line(stack, line, sizeof(line), &cur)) {
        strip_comment(line);
        char *ltokens[MAX_TOKENS];
        int lcount = tokenize(line, ltokens, MAX_TOKENS);
        if (lcount == 0) continue;
        if (strcmp(ltokens[0], ".endm") == 0) break;
        if (macro.line_count + 1 > macro.line_cap) {
            size_t next = (macro.line_cap == 0) ? 8 : macro.line_cap * 2;
            MacroLine *mem = (MacroLine *)realloc(macro.lines, next * sizeof(MacroLine));
            if (!mem) return 0;
            macro.lines = mem;
            macro.line_cap = next;
        }
        MacroLine *ml = &macro.lines[macro.line_count++];
        ml->count = lcount;
        ml->tokens = (char **)calloc((size_t)lcount, sizeof(char *));
        if (!ml->tokens) return 0;
        for (int i = 0; i < lcount; i++) {
            ml->tokens[i] = strdup(ltokens[i]);
            if (!ml->tokens[i]) return 0;
        }
    }

    if (!macro_add(macros, macro)) return 0;
    return 1;
}

static int macro_expand(LineStack *stack, Macro *macro, char *tokens[], int count) {
    size_t out_count = macro->line_count;
    char **lines = (char **)calloc(out_count, sizeof(char *));
    if (!lines) return 0;
    for (size_t i = 0; i < out_count; i++) {
        MacroLine *ml = &macro->lines[i];
        size_t cap = 0;
        size_t len = 0;
        char *buf = NULL;
        for (int t = 0; t < ml->count; t++) {
            const char *tok = ml->tokens[t];
            const char *rep = tok;
            for (int p = 0; p < macro->param_count; p++) {
                if (strcmp(tok, macro->params[p]) == 0) {
                    int arg_index = p + 1;
                    if (arg_index < count) rep = tokens[arg_index];
                    else rep = "";
                    break;
                }
            }
            size_t rlen = strlen(rep);
            size_t need = len + rlen + 2;
            if (need > cap) {
                size_t next = (cap == 0) ? 64 : cap * 2;
                while (next < need) next *= 2;
                char *mem = (char *)realloc(buf, next);
                if (!mem) { free(buf); return 0; }
                buf = mem;
                cap = next;
            }
            if (len > 0) buf[len++] = ' ';
            memcpy(buf + len, rep, rlen);
            len += rlen;
            buf[len] = '\0';
        }
        lines[i] = buf ? buf : strdup("");
        if (!lines[i]) return 0;
    }
    LineSource src = {0};
    src.kind = SRC_LINES;
    src.lines = lines;
    src.count = out_count;
    src.index = 0;
    src.line_no = 0;
    src.dir[0] = '\0';
    return stack_push(stack, src);
}
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
    if (strcmp(tokens[0], ".zero") == 0 && count >= 2) {
        int64_t v; if (!parse_int(tokens[1], &v)) return 0;
        if (v < 0) return 0;
        for (int64_t i = 0; i < v; i++) {
            if (kind != SEC_BSS) buf_write_u8(&sec->buf, 0);
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

int first_pass(FILE *f, const char *path, LabelTable *labels, const AsmOptions *opt) {
    char line[MAX_LINE];
    SectionKind cur = SEC_TEXT;
    uint64_t text_pc = 0, data_pc = 0, bss_pc = 0;
    bool skip_unreachable = false;
    LineStack stack = {0};
    MacroTable macros = {0};
    LineSource root = {0};
    root.kind = SRC_FILE;
    root.file = f;
    root.own_file = false;
    root.line_no = 0;
    get_dirname(path, root.dir, sizeof(root.dir));
    if (!stack_push(&stack, root)) return 0;
    LineSource *src = NULL;
    while (stack_next_line(&stack, line, sizeof(line), &src)) {
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

        if (strcmp(tokens[0], ".include") == 0 && count >= 2) {
            char inc_path[512];
            if (!resolve_include_path(src ? src->dir : ".", tokens[1], inc_path, sizeof(inc_path))) { stack_free(&stack); macro_table_free(&macros); return 0; }
            FILE *inc = fopen(inc_path, "r");
            if (!inc) { stack_free(&stack); macro_table_free(&macros); return 0; }
            LineSource inc_src = {0};
            inc_src.kind = SRC_FILE;
            inc_src.file = inc;
            inc_src.own_file = true;
            inc_src.line_no = 0;
            get_dirname(inc_path, inc_src.dir, sizeof(inc_src.dir));
            if (!stack_push(&stack, inc_src)) { fclose(inc); stack_free(&stack); macro_table_free(&macros); return 0; }
            continue;
        }
        if (strcmp(tokens[0], ".macro") == 0) {
            if (!macro_capture(&stack, &macros, tokens, count)) { stack_free(&stack); macro_table_free(&macros); return 0; }
            continue;
        }
        Macro *macro = macro_find(&macros, tokens[0]);
        if (macro) {
            if (!macro_expand(&stack, macro, tokens, count)) { stack_free(&stack); macro_table_free(&macros); return 0; }
            continue;
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
            } else if (strcmp(tokens[0], ".zero") == 0 && count >= 2) {
                int64_t v; if (!parse_int(tokens[1], &v)) return 0;
                if (v < 0) return 0;
                *pc += (uint64_t)v;
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
    stack_free(&stack);
    macro_table_free(&macros);
    return 1;
}

int assemble_file(const char *in_path, const char *out_path, bool elf_output, const AsmOptions *opt) {
    FILE *f = fopen(in_path, "r");
    if (!f) return 0;
    LabelTable labels = {0};
    if (!first_pass(f, in_path, &labels, opt)) { fclose(f); return 0; }
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
    LineStack stack = {0};
    MacroTable macros = {0};
    LineSource root = {0};
    root.kind = SRC_FILE;
    root.file = f;
    root.own_file = false;
    root.line_no = 0;
    get_dirname(in_path, root.dir, sizeof(root.dir));
    if (!stack_push(&stack, root)) { fclose(f); return 0; }
    LineSource *src = NULL;
    while (stack_next_line(&stack, line, sizeof(line), &src)) {
        line_no = src ? src->line_no : line_no + 1;
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

        if (strcmp(tokens[0], ".include") == 0 && count >= 2) {
            char inc_path[512];
            if (!resolve_include_path(src ? src->dir : ".", tokens[1], inc_path, sizeof(inc_path))) { ok = 0; break; }
            FILE *inc = fopen(inc_path, "r");
            if (!inc) { ok = 0; break; }
            LineSource inc_src = {0};
            inc_src.kind = SRC_FILE;
            inc_src.file = inc;
            inc_src.own_file = true;
            inc_src.line_no = 0;
            get_dirname(inc_path, inc_src.dir, sizeof(inc_src.dir));
            if (!stack_push(&stack, inc_src)) { fclose(inc); ok = 0; break; }
            continue;
        }
        if (strcmp(tokens[0], ".macro") == 0) {
            if (!macro_capture(&stack, &macros, tokens, count)) { ok = 0; break; }
            continue;
        }
        Macro *macro = macro_find(&macros, tokens[0]);
        if (macro) {
            if (!macro_expand(&stack, macro, tokens, count)) { ok = 0; break; }
            continue;
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
    stack_free(&stack);
    macro_table_free(&macros);

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
