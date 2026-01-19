#include "codegen.h"
#include <stdlib.h>
#include <string.h>

static const char *temp_regs[] = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8"
};

static const char *scratch_regs[] = {
    "t9", "t10"
};

static const char *arg_regs[] = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
};

typedef struct {
    const char *name;
    int needs_ra_save;
    int max_temp;
    struct LocalInfo *locals;
    size_t local_count;
    size_t local_cap;
    size_t local_bytes;
} FuncInfo;

typedef struct LocalInfo {
    const char *name;
    size_t size;
    int offset;
} LocalInfo;

static FuncInfo *find_func(FuncInfo *funcs, size_t count, const char *name) {
    for (size_t i = 0; i < count; i++) {
        if (funcs[i].name == name) return &funcs[i];
        if (funcs[i].name && name && strcmp(funcs[i].name, name) == 0) return &funcs[i];
    }
    return NULL;
}

static int find_name(const char **names, size_t count, const char *name) {
    for (size_t i = 0; i < count; i++) {
        if (names[i] == name) return (int)i;
        if (names[i] && name && strcmp(names[i], name) == 0) return (int)i;
    }
    return -1;
}

static int find_local(const LocalInfo *locals, size_t count, const char *name) {
    for (size_t i = 0; i < count; i++) {
        if (locals[i].name == name) return (int)i;
        if (locals[i].name && name && strcmp(locals[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int add_local(FuncInfo *info, const char *name, size_t size) {
    if (!info || !name) return 0;
    int idx = find_local(info->locals, info->local_count, name);
    if (idx >= 0) {
        size_t aligned = (size + 7) & ~((size_t)7);
        if (aligned > info->locals[idx].size) info->locals[idx].size = aligned;
        return 1;
    }
    if (info->local_count + 1 > info->local_cap) {
        size_t next = (info->local_cap == 0) ? 8 : info->local_cap * 2;
        LocalInfo *mem = (LocalInfo *)realloc(info->locals, next * sizeof(LocalInfo));
        if (!mem) return 0;
        info->locals = mem;
        info->local_cap = next;
    }
    size_t aligned = (size + 7) & ~((size_t)7);
    info->locals[info->local_count].name = name;
    info->locals[info->local_count].size = aligned;
    info->locals[info->local_count].offset = 0;
    info->local_count++;
    return 1;
}

static void finalize_locals(FuncInfo *info) {
    if (!info) return;
    size_t offset = 0;
    for (size_t i = 0; i < info->local_count; i++) {
        info->locals[i].offset = (int)offset;
        offset += info->locals[i].size;
    }
    info->local_bytes = offset;
}

static void update_max_temp(FuncInfo *info, int temp) {
    if (!info) return;
    if (temp > info->max_temp) info->max_temp = temp;
}

static int temp_reg_count(void) {
    return (int)(sizeof(temp_regs) / sizeof(temp_regs[0]));
}

static int local_offset(const FuncInfo *info, int ra_size, int spill_count, const char *name) {
    if (!info || !name) return -1;
    int idx = find_local(info->locals, info->local_count, name);
    if (idx < 0) return -1;
    return ra_size + spill_count * 8 + info->locals[idx].offset;
}

static int spill_offset(int temp, int ra_size, int reg_count) {
    return ra_size + (temp - reg_count) * 8;
}

static char *dup_error(const char *msg) {
    size_t len = strlen(msg) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, msg, len);
    return out;
}

static const char *temp_reg_name(int temp, char **out_error) {
    size_t count = sizeof(temp_regs) / sizeof(temp_regs[0]);
    if (temp < 0 || (size_t)temp >= count) {
        if (out_error && !*out_error) *out_error = dup_error("error: too many temporaries for register allocation");
        return NULL;
    }
    return temp_regs[temp];
}

static const char *binop_mnemonic(BinOp op) {
    switch (op) {
        case BIN_ADD: return "add";
        case BIN_SUB: return "sub";
        case BIN_MUL: return "mul";
        case BIN_DIV: return "div";
        case BIN_LT: return "slt";
        case BIN_GT: return "slt";
        case BIN_EQ: return "xor";
        case BIN_NEQ: return "xor";
        case BIN_LTE: return "slt";
        case BIN_GTE: return "slt";
        case BIN_LOGAND: return "and";
        case BIN_LOGOR: return "or";
        default: return "add";
    }
}

int codegen_emit_asm(const IRProgram *ir, FILE *out, int emit_start, char **out_error) {
    if (out_error) *out_error = NULL;
    if (!ir || !out) return 0;

    const char **globals = NULL;
    size_t global_count = 0;
    size_t global_cap = 0;
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        if (inst->op == IR_GLOBAL_INT || inst->op == IR_GLOBAL_CHAR || inst->op == IR_GLOBAL_STR || inst->op == IR_GLOBAL_INT_ARR || inst->op == IR_GLOBAL_BYTES) {
            if (find_name(globals, global_count, inst->name) < 0) {
                if (global_count + 1 > global_cap) {
                    size_t next = (global_cap == 0) ? 8 : global_cap * 2;
                    const char **mem = (const char **)realloc(globals, next * sizeof(char *));
                    if (!mem) { free(globals); if (out_error) *out_error = dup_error("error: out of memory"); return 0; }
                    globals = mem;
                    global_cap = next;
                }
                globals[global_count++] = inst->name;
            }
        }
    }

    FuncInfo *funcs = NULL;
    size_t func_count = 0;
    FuncInfo *current_info = NULL;
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        if (inst->op == IR_FUNC) {
            current_info = find_func(funcs, func_count, inst->name);
            if (!current_info) {
                FuncInfo *mem = (FuncInfo *)realloc(funcs, (func_count + 1) * sizeof(FuncInfo));
                if (!mem) { free(funcs); free(globals); if (out_error) *out_error = dup_error("error: out of memory"); return 0; }
                funcs = mem;
                funcs[func_count].name = inst->name;
                funcs[func_count].needs_ra_save = 0;
                funcs[func_count].max_temp = -1;
                funcs[func_count].locals = NULL;
                funcs[func_count].local_count = 0;
                funcs[func_count].local_cap = 0;
                funcs[func_count].local_bytes = 0;
                current_info = &funcs[func_count++];
            }
            continue;
        }
        if (!current_info) continue;
        if (inst->op == IR_CALL) current_info->needs_ra_save = 1;
        if (inst->op == IR_LOCAL_ALLOC && inst->name) {
            if (!add_local(current_info, inst->name, (size_t)inst->len)) { free(funcs); free(globals); if (out_error) *out_error = dup_error("error: out of memory"); return 0; }
        }
        if (inst->op == IR_ADDR && inst->name && find_name(globals, global_count, inst->name) < 0) {
            if (!add_local(current_info, inst->name, 8)) { free(funcs); free(globals); if (out_error) *out_error = dup_error("error: out of memory"); return 0; }
        }
        update_max_temp(current_info, inst->dst);
        update_max_temp(current_info, inst->lhs);
        update_max_temp(current_info, inst->rhs);
        if (inst->op == IR_CALL && inst->args) {
            for (int a = 0; a < inst->argc; a++) update_max_temp(current_info, inst->args[a]);
        }
    }

    for (size_t i = 0; i < func_count; i++) {
        finalize_locals(&funcs[i]);
    }

    int has_data = 0;
    for (size_t i = 0; i < ir->count; i++) {
        if (ir->insts[i].op == IR_GLOBAL_INT || ir->insts[i].op == IR_GLOBAL_CHAR || ir->insts[i].op == IR_GLOBAL_STR || ir->insts[i].op == IR_GLOBAL_INT_ARR || ir->insts[i].op == IR_GLOBAL_BYTES) { has_data = 1; break; }
    }
    if (has_data) {
        fprintf(out, ".data\n");
        for (size_t i = 0; i < ir->count; i++) {
            const IRInst *inst = &ir->insts[i];
            if (inst->op == IR_GLOBAL_INT) {
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                fprintf(out, "  .dword %ld\n", inst->imm);
            } else if (inst->op == IR_GLOBAL_CHAR) {
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                fprintf(out, "  .byte %u\n", (unsigned char)inst->imm);
            } else if (inst->op == IR_GLOBAL_INT_ARR) {
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                size_t count = (size_t)inst->imm;
                for (size_t b = 0; b < count; b++) {
                    long v = (b < inst->val_count && inst->values) ? inst->values[b] : 0;
                    fprintf(out, "  .dword %ld\n", v);
                }
            } else if (inst->op == IR_GLOBAL_STR) {
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                for (size_t b = 0; b < inst->len; b++) {
                    fprintf(out, "  .byte %u\n", (unsigned char)inst->data[b]);
                }
                fprintf(out, "  .byte 0\n");
            } else if (inst->op == IR_GLOBAL_BYTES) {
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                for (size_t b = 0; b < inst->len; b++) {
                    fprintf(out, "  .byte %u\n", (unsigned char)inst->data[b]);
                }
            }
        }
    }

    fprintf(out, ".text\n");
    if (emit_start) {
        fprintf(out, "start:\n");
        fprintf(out, "  jal ra, main\n");
        fprintf(out, "  mov r10, a0\n");
        fprintf(out, "  li r17, 3\n");
        fprintf(out, "  ecall\n");
    }

    const FuncInfo *current = NULL;
    int cur_spill_count = 0;
    int cur_ra_size = 0;
    int cur_frame_size = 0;
    int reg_count = temp_reg_count();
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        switch (inst->op) {
            case IR_GLOBAL_INT:
            case IR_GLOBAL_CHAR:
            case IR_GLOBAL_INT_ARR:
            case IR_GLOBAL_STR:
            case IR_GLOBAL_BYTES:
            case IR_LOCAL_ALLOC:
                break;
            case IR_FUNC:
                current = find_func(funcs, func_count, inst->name);
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                if (current) {
                    cur_ra_size = current->needs_ra_save ? 8 : 0;
                    int max_temp = current->max_temp;
                    cur_spill_count = (max_temp + 1 > reg_count) ? (max_temp + 1 - reg_count) : 0;
                    cur_frame_size = cur_ra_size + cur_spill_count * 8 + (int)current->local_bytes;
                    if (cur_frame_size > 0) {
                        fprintf(out, "  addi sp, sp, -%d\n", cur_frame_size);
                        if (current->needs_ra_save) {
                            fprintf(out, "  st r31, 0, sp\n");
                        }
                    }
                } else {
                    cur_spill_count = 0;
                    cur_ra_size = 0;
                    cur_frame_size = 0;
                }
                break;
            case IR_PARAM: {
                if (inst->imm < 0 || inst->imm > 7) {
                    if (out_error && !*out_error) *out_error = dup_error("error: too many parameters");
                    return 0;
                }
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", arg_regs[inst->imm], off);
                } else {
                    const char *rd = temp_reg_name(inst->dst, out_error);
                    if (!rd) return 0;
                    fprintf(out, "  mov %s, %s\n", rd, arg_regs[inst->imm]);
                }
                break;
            }
            case IR_CONST: {
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    const char *rd = scratch_regs[0];
                    fprintf(out, "  li %s, %ld\n", rd, inst->imm);
                    fprintf(out, "  st %s, %d, sp\n", rd, off);
                } else {
                    const char *rd = temp_reg_name(inst->dst, out_error);
                    if (!rd) return 0;
                    fprintf(out, "  li %s, %ld\n", rd, inst->imm);
                }
                break;
            }
            case IR_ADDR: {
                const char *rd = (inst->dst >= reg_count) ? scratch_regs[0] : temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                if (inst->name && current && find_name(globals, global_count, inst->name) < 0) {
                    int off = local_offset(current, cur_ra_size, cur_spill_count, inst->name);
                    if (off < 0) { if (out_error && !*out_error) *out_error = dup_error("error: unknown local address"); return 0; }
                    fprintf(out, "  addi %s, sp, %d\n", rd, off);
                } else {
                    fprintf(out, "  li %s, %s\n", rd, inst->name ? inst->name : "<anon>");
                }
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", rd, off);
                }
                break;
            }
            case IR_LOAD: {
                const char *addr = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    addr = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", addr, off);
                } else {
                    addr = temp_reg_name(inst->lhs, out_error);
                    if (!addr) return 0;
                }
                const char *rd = (inst->dst >= reg_count) ? scratch_regs[1] : temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                fprintf(out, "  ld %s, 0, %s\n", rd, addr);
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", rd, off);
                }
                break;
            }
            case IR_LOAD8: {
                const char *addr = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    addr = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", addr, off);
                } else {
                    addr = temp_reg_name(inst->lhs, out_error);
                    if (!addr) return 0;
                }
                const char *rd = (inst->dst >= reg_count) ? scratch_regs[1] : temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                fprintf(out, "  ldb %s, 0, %s\n", rd, addr);
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", rd, off);
                }
                break;
            }
            case IR_STORE: {
                const char *addr = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    addr = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", addr, off);
                } else {
                    addr = temp_reg_name(inst->lhs, out_error);
                    if (!addr) return 0;
                }
                const char *val = NULL;
                if (inst->rhs >= reg_count) {
                    int off = spill_offset(inst->rhs, cur_ra_size, reg_count);
                    val = scratch_regs[1];
                    fprintf(out, "  ld %s, %d, sp\n", val, off);
                } else {
                    val = temp_reg_name(inst->rhs, out_error);
                    if (!val) return 0;
                }
                fprintf(out, "  st %s, 0, %s\n", val, addr);
                break;
            }
            case IR_STORE8: {
                const char *addr = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    addr = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", addr, off);
                } else {
                    addr = temp_reg_name(inst->lhs, out_error);
                    if (!addr) return 0;
                }
                const char *val = NULL;
                if (inst->rhs >= reg_count) {
                    int off = spill_offset(inst->rhs, cur_ra_size, reg_count);
                    val = scratch_regs[1];
                    fprintf(out, "  ld %s, %d, sp\n", val, off);
                } else {
                    val = temp_reg_name(inst->rhs, out_error);
                    if (!val) return 0;
                }
                fprintf(out, "  stb %s, 0, %s\n", val, addr);
                break;
            }
            case IR_BIN: {
                const char *rs1 = NULL;
                const char *rs2 = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    rs1 = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", rs1, off);
                } else {
                    rs1 = temp_reg_name(inst->lhs, out_error);
                    if (!rs1) return 0;
                }
                if (inst->rhs >= reg_count) {
                    int off = spill_offset(inst->rhs, cur_ra_size, reg_count);
                    rs2 = scratch_regs[1];
                    fprintf(out, "  ld %s, %d, sp\n", rs2, off);
                } else {
                    rs2 = temp_reg_name(inst->rhs, out_error);
                    if (!rs2) return 0;
                }
                const char *rd = (inst->dst >= reg_count) ? scratch_regs[0] : temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                switch (inst->bin_op) {
                    case BIN_LT:
                        fprintf(out, "  slt %s, %s, %s\n", rd, rs1, rs2);
                        break;
                    case BIN_GT:
                        fprintf(out, "  slt %s, %s, %s\n", rd, rs2, rs1);
                        break;
                    case BIN_LTE:
                        fprintf(out, "  slt %s, %s, %s\n", rd, rs2, rs1);
                        fprintf(out, "  xori %s, %s, 1\n", rd, rd);
                        break;
                    case BIN_GTE:
                        fprintf(out, "  slt %s, %s, %s\n", rd, rs1, rs2);
                        fprintf(out, "  xori %s, %s, 1\n", rd, rd);
                        break;
                    case BIN_EQ:
                        fprintf(out, "  xor %s, %s, %s\n", rd, rs1, rs2);
                        fprintf(out, "  sltiu %s, %s, 1\n", rd, rd);
                        break;
                    case BIN_NEQ:
                        fprintf(out, "  xor %s, %s, %s\n", rd, rs1, rs2);
                        fprintf(out, "  sltu %s, zero, %s\n", rd, rd);
                        break;
                    default:
                        fprintf(out, "  %s %s, %s, %s\n", binop_mnemonic(inst->bin_op), rd, rs1, rs2);
                        break;
                }
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", rd, off);
                }
                break;
            }
            case IR_MOV: {
                const char *rs = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    rs = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", rs, off);
                } else {
                    rs = temp_reg_name(inst->lhs, out_error);
                    if (!rs) return 0;
                }
                if (inst->dst >= reg_count) {
                    int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                    fprintf(out, "  st %s, %d, sp\n", rs, off);
                } else {
                    const char *rd = temp_reg_name(inst->dst, out_error);
                    if (!rd) return 0;
                    fprintf(out, "  mov %s, %s\n", rd, rs);
                }
                break;
            }
            case IR_CALL: {
                if (inst->argc > 8) {
                    if (out_error && !*out_error) *out_error = dup_error("error: too many call arguments");
                    return 0;
                }
                for (int a = 0; a < inst->argc; a++) {
                    const char *rs = NULL;
                    if (inst->args[a] >= reg_count) {
                        int off = spill_offset(inst->args[a], cur_ra_size, reg_count);
                        rs = scratch_regs[0];
                        fprintf(out, "  ld %s, %d, sp\n", rs, off);
                    } else {
                        rs = temp_reg_name(inst->args[a], out_error);
                        if (!rs) return 0;
                    }
                    fprintf(out, "  mov %s, %s\n", arg_regs[a], rs);
                }
                fprintf(out, "  jal ra, %s\n", inst->name ? inst->name : "<anon>");
                if (inst->dst >= 0) {
                    if (inst->dst >= reg_count) {
                        int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                        fprintf(out, "  mov %s, a0\n", scratch_regs[0]);
                        fprintf(out, "  st %s, %d, sp\n", scratch_regs[0], off);
                    } else {
                        const char *rd = temp_reg_name(inst->dst, out_error);
                        if (!rd) return 0;
                        fprintf(out, "  mov %s, a0\n", rd);
                    }
                }
                break;
            }
            case IR_WRITE: {
                const char *addr = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    addr = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", addr, off);
                } else {
                    addr = temp_reg_name(inst->lhs, out_error);
                    if (!addr) return 0;
                }
                fprintf(out, "  li r10, 1\n");
                fprintf(out, "  mov r11, %s\n", addr);
                fprintf(out, "  li r12, %zu\n", inst->len);
                fprintf(out, "  li r17, 1\n");
                fprintf(out, "  ecall\n");
                if (inst->dst >= 0) {
                    if (inst->dst >= reg_count) {
                        int off = spill_offset(inst->dst, cur_ra_size, reg_count);
                        fprintf(out, "  mov %s, a0\n", scratch_regs[0]);
                        fprintf(out, "  st %s, %d, sp\n", scratch_regs[0], off);
                    } else {
                        const char *rd = temp_reg_name(inst->dst, out_error);
                        if (!rd) return 0;
                        fprintf(out, "  mov %s, a0\n", rd);
                    }
                }
                break;
            }
            case IR_RET: {
                const char *rs = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    rs = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", rs, off);
                } else {
                    rs = temp_reg_name(inst->lhs, out_error);
                    if (!rs) return 0;
                }
                fprintf(out, "  mov a0, %s\n", rs);
                if (current && current->needs_ra_save) {
                    fprintf(out, "  ld r31, 0, sp\n");
                }
                if (cur_frame_size > 0) {
                    fprintf(out, "  addi sp, sp, %d\n", cur_frame_size);
                }
                fprintf(out, "  ret\n");
                break;
            }
            case IR_EXIT: {
                const char *rs = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    rs = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", rs, off);
                } else {
                    rs = temp_reg_name(inst->lhs, out_error);
                    if (!rs) return 0;
                }
                fprintf(out, "  mov r10, %s\n", rs);
                fprintf(out, "  li r17, 3\n");
                fprintf(out, "  ecall\n");
                break;
            }
            case IR_LABEL:
                fprintf(out, ".L%d:\n", inst->label);
                break;
            case IR_JMP:
                fprintf(out, "  j .L%d\n", inst->label);
                break;
            case IR_BZ: {
                const char *rs = NULL;
                if (inst->lhs >= reg_count) {
                    int off = spill_offset(inst->lhs, cur_ra_size, reg_count);
                    rs = scratch_regs[0];
                    fprintf(out, "  ld %s, %d, sp\n", rs, off);
                } else {
                    rs = temp_reg_name(inst->lhs, out_error);
                    if (!rs) return 0;
                }
                fprintf(out, "  beq %s, zero, .L%d\n", rs, inst->label);
                break;
            }
            default:
                if (out_error && !*out_error) *out_error = dup_error("error: unsupported IR opcode");
                return 0;
        }
    }
    if (funcs) {
        for (size_t i = 0; i < func_count; i++) free(funcs[i].locals);
    }
    free(funcs);
    free(globals);

    return 1;
}
