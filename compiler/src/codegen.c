#include "codegen.h"
#include <stdlib.h>
#include <string.h>

static const char *temp_regs[] = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "t10"
};

static const char *arg_regs[] = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
};

typedef struct {
    const char *name;
    int needs_ra_save;
} FuncInfo;

static FuncInfo *find_func(FuncInfo *funcs, size_t count, const char *name) {
    for (size_t i = 0; i < count; i++) {
        if (funcs[i].name == name) return &funcs[i];
        if (funcs[i].name && name && strcmp(funcs[i].name, name) == 0) return &funcs[i];
    }
    return NULL;
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
        default: return "add";
    }
}

int codegen_emit_asm(const IRProgram *ir, FILE *out, char **out_error) {
    if (out_error) *out_error = NULL;
    if (!ir || !out) return 0;

    FuncInfo *funcs = NULL;
    size_t func_count = 0;
    const char *cur_name = NULL;
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        if (inst->op == IR_FUNC) {
            cur_name = inst->name;
            if (!find_func(funcs, func_count, cur_name)) {
                FuncInfo *mem = (FuncInfo *)realloc(funcs, (func_count + 1) * sizeof(FuncInfo));
                if (!mem) { free(funcs); if (out_error) *out_error = dup_error("error: out of memory"); return 0; }
                funcs = mem;
                funcs[func_count].name = cur_name;
                funcs[func_count].needs_ra_save = 0;
                func_count++;
            }
        } else if (inst->op == IR_CALL) {
            FuncInfo *info = find_func(funcs, func_count, cur_name);
            if (info) info->needs_ra_save = 1;
        }
    }

    fprintf(out, ".text\n");
    fprintf(out, "start:\n");
    fprintf(out, "  jal ra, main\n");
    fprintf(out, "  mov r10, a0\n");
    fprintf(out, "  li r17, 3\n");
    fprintf(out, "  ecall\n");

    const FuncInfo *current = NULL;
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        switch (inst->op) {
            case IR_FUNC:
                current = find_func(funcs, func_count, inst->name);
                fprintf(out, "%s:\n", inst->name ? inst->name : "<anon>");
                if (current && current->needs_ra_save) {
                    fprintf(out, "  addi sp, sp, -8\n");
                    fprintf(out, "  st r31, 0, sp\n");
                }
                break;
            case IR_PARAM: {
                if (inst->imm < 0 || inst->imm > 7) {
                    if (out_error && !*out_error) *out_error = dup_error("error: too many parameters");
                    return 0;
                }
                const char *rd = temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                fprintf(out, "  mov %s, %s\n", rd, arg_regs[inst->imm]);
                break;
            }
            case IR_CONST: {
                const char *rd = temp_reg_name(inst->dst, out_error);
                if (!rd) return 0;
                fprintf(out, "  li %s, %ld\n", rd, inst->imm);
                break;
            }
            case IR_BIN: {
                const char *rd = temp_reg_name(inst->dst, out_error);
                const char *rs1 = temp_reg_name(inst->lhs, out_error);
                const char *rs2 = temp_reg_name(inst->rhs, out_error);
                if (!rd || !rs1 || !rs2) return 0;
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
                break;
            }
            case IR_MOV: {
                const char *rd = temp_reg_name(inst->dst, out_error);
                const char *rs = temp_reg_name(inst->lhs, out_error);
                if (!rd || !rs) return 0;
                fprintf(out, "  mov %s, %s\n", rd, rs);
                break;
            }
            case IR_CALL: {
                if (inst->argc > 8) {
                    if (out_error && !*out_error) *out_error = dup_error("error: too many call arguments");
                    return 0;
                }
                for (int a = 0; a < inst->argc; a++) {
                    const char *rs = temp_reg_name(inst->args[a], out_error);
                    if (!rs) return 0;
                    fprintf(out, "  mov %s, %s\n", arg_regs[a], rs);
                }
                fprintf(out, "  jal ra, %s\n", inst->name ? inst->name : "<anon>");
                if (inst->dst >= 0) {
                    const char *rd = temp_reg_name(inst->dst, out_error);
                    if (!rd) return 0;
                    fprintf(out, "  mov %s, a0\n", rd);
                }
                break;
            }
            case IR_RET: {
                const char *rs = temp_reg_name(inst->lhs, out_error);
                if (!rs) return 0;
                fprintf(out, "  mov a0, %s\n", rs);
                if (current && current->needs_ra_save) {
                    fprintf(out, "  ld r31, 0, sp\n");
                    fprintf(out, "  addi sp, sp, 8\n");
                }
                fprintf(out, "  ret\n");
                break;
            }
            case IR_LABEL:
                fprintf(out, ".L%d:\n", inst->label);
                break;
            case IR_JMP:
                fprintf(out, "  j .L%d\n", inst->label);
                break;
            case IR_BZ: {
                const char *rs = temp_reg_name(inst->lhs, out_error);
                if (!rs) return 0;
                fprintf(out, "  beq %s, zero, .L%d\n", rs, inst->label);
                break;
            }
            default:
                if (out_error && !*out_error) *out_error = dup_error("error: unsupported IR opcode");
                return 0;
        }
    }
    free(funcs);

    return 1;
}
