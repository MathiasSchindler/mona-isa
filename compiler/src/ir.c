#include "ir.h"
#include <stdlib.h>
#include <string.h>

static int ir_push(IRProgram *ir, IRInst inst) {
    if (ir->count + 1 > ir->cap) {
        size_t next = (ir->cap == 0) ? 64 : ir->cap * 2;
        IRInst *mem = (IRInst *)realloc(ir->insts, next * sizeof(IRInst));
        if (!mem) return 0;
        ir->insts = mem;
        ir->cap = next;
    }
    ir->insts[ir->count++] = inst;
    return 1;
}

void ir_init(IRProgram *ir) {
    ir->insts = NULL;
    ir->count = 0;
    ir->cap = 0;
    ir->temp_count = 0;
    ir->label_count = 0;
}

void ir_free(IRProgram *ir) {
    if (ir->insts) {
        for (size_t i = 0; i < ir->count; i++) {
            if (ir->insts[i].op == IR_CALL) free(ir->insts[i].args);
            if (ir->insts[i].op == IR_GLOBAL_STR) free(ir->insts[i].data);
            if (ir->insts[i].op == IR_GLOBAL_INT_ARR) free(ir->insts[i].values);
        }
    }
    free(ir->insts);
    ir->insts = NULL;
    ir->count = 0;
    ir->cap = 0;
    ir->temp_count = 0;
    ir->label_count = 0;
}

int ir_new_temp(IRProgram *ir) {
    return ir->temp_count++;
}

int ir_emit_const(IRProgram *ir, long value) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_CONST;
    inst.dst = temp;
    inst.imm = value;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

int ir_emit_bin(IRProgram *ir, BinOp op, int lhs, int rhs) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_BIN;
    inst.dst = temp;
    inst.lhs = lhs;
    inst.rhs = rhs;
    inst.bin_op = op;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

void ir_emit_mov(IRProgram *ir, int dst, int src) {
    IRInst inst = {0};
    inst.op = IR_MOV;
    inst.dst = dst;
    inst.lhs = src;
    ir_push(ir, inst);
}

int ir_emit_addr(IRProgram *ir, const char *name) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_ADDR;
    inst.dst = temp;
    inst.name = name;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

int ir_emit_load(IRProgram *ir, int addr_temp) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_LOAD;
    inst.dst = temp;
    inst.lhs = addr_temp;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

void ir_emit_store(IRProgram *ir, int addr_temp, int value_temp) {
    IRInst inst = {0};
    inst.op = IR_STORE;
    inst.lhs = addr_temp;
    inst.rhs = value_temp;
    ir_push(ir, inst);
}

void ir_emit_global_int(IRProgram *ir, const char *name, long value) {
    IRInst inst = {0};
    inst.op = IR_GLOBAL_INT;
    inst.name = name;
    inst.imm = value;
    ir_push(ir, inst);
}

void ir_emit_global_int_arr(IRProgram *ir, const char *name, const long *values, size_t count, size_t init_count) {
    IRInst inst = {0};
    inst.op = IR_GLOBAL_INT_ARR;
    inst.name = name;
    inst.imm = (long)count;
    inst.val_count = init_count;
    if (init_count > 0 && values) {
        inst.values = (long *)malloc(init_count * sizeof(long));
        if (!inst.values) return;
        for (size_t i = 0; i < init_count; i++) inst.values[i] = values[i];
    }
    ir_push(ir, inst);
}

void ir_emit_global_str(IRProgram *ir, const char *name, const char *data, size_t len) {
    IRInst inst = {0};
    inst.op = IR_GLOBAL_STR;
    inst.name = name;
    inst.len = len;
    if (len > 0) {
        inst.data = (char *)malloc(len);
        if (!inst.data) return;
        memcpy(inst.data, data, len);
    }
    ir_push(ir, inst);
}

int ir_emit_write(IRProgram *ir, int addr_temp, size_t len) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_WRITE;
    inst.dst = temp;
    inst.lhs = addr_temp;
    inst.len = len;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

void ir_emit_ret(IRProgram *ir, int value_temp) {
    IRInst inst = {0};
    inst.op = IR_RET;
    inst.lhs = value_temp;
    ir_push(ir, inst);
}

void ir_emit_func(IRProgram *ir, const char *name) {
    IRInst inst = {0};
    inst.op = IR_FUNC;
    inst.name = name;
    ir_push(ir, inst);
}

void ir_emit_param(IRProgram *ir, int index, int dst) {
    IRInst inst = {0};
    inst.op = IR_PARAM;
    inst.dst = dst;
    inst.imm = index;
    ir_push(ir, inst);
}

int ir_emit_call(IRProgram *ir, const char *name, const int *args, int argc) {
    int temp = ir_new_temp(ir);
    IRInst inst = {0};
    inst.op = IR_CALL;
    inst.name = name;
    inst.argc = argc;
    if (argc > 0) {
        inst.args = (int *)malloc((size_t)argc * sizeof(int));
        if (!inst.args) return -1;
        for (int i = 0; i < argc; i++) inst.args[i] = args[i];
    }
    inst.dst = temp;
    if (!ir_push(ir, inst)) { free(inst.args); return -1; }
    return temp;
}

int ir_new_label(IRProgram *ir) {
    return ir->label_count++;
}

void ir_emit_label(IRProgram *ir, int label) {
    IRInst inst = {0};
    inst.op = IR_LABEL;
    inst.label = label;
    ir_push(ir, inst);
}

void ir_emit_jmp(IRProgram *ir, int label) {
    IRInst inst = {0};
    inst.op = IR_JMP;
    inst.label = label;
    ir_push(ir, inst);
}

void ir_emit_bz(IRProgram *ir, int cond_temp, int label) {
    IRInst inst = {0};
    inst.op = IR_BZ;
    inst.lhs = cond_temp;
    inst.label = label;
    ir_push(ir, inst);
}

static const char *binop_name(BinOp op) {
    switch (op) {
        case BIN_ADD: return "add";
        case BIN_SUB: return "sub";
        case BIN_MUL: return "mul";
        case BIN_DIV: return "div";
        case BIN_EQ: return "eq";
        case BIN_NEQ: return "neq";
        case BIN_LT: return "lt";
        case BIN_LTE: return "lte";
        case BIN_GT: return "gt";
        case BIN_GTE: return "gte";
        case BIN_LOGAND: return "logand";
        case BIN_LOGOR: return "logor";
        default: return "?";
    }
}

void ir_print(const IRProgram *ir, FILE *out) {
    for (size_t i = 0; i < ir->count; i++) {
        const IRInst *inst = &ir->insts[i];
        switch (inst->op) {
            case IR_CONST:
                fprintf(out, "t%d = const %ld\n", inst->dst, inst->imm);
                break;
            case IR_BIN:
                fprintf(out, "t%d = %s t%d, t%d\n", inst->dst, binop_name(inst->bin_op), inst->lhs, inst->rhs);
                break;
            case IR_ADDR:
                fprintf(out, "t%d = addr %s\n", inst->dst, inst->name ? inst->name : "<anon>");
                break;
            case IR_LOAD:
                fprintf(out, "t%d = load t%d\n", inst->dst, inst->lhs);
                break;
            case IR_STORE:
                fprintf(out, "store t%d, t%d\n", inst->lhs, inst->rhs);
                break;
            case IR_GLOBAL_INT:
                fprintf(out, "global %s = %ld\n", inst->name ? inst->name : "<anon>", inst->imm);
                break;
            case IR_GLOBAL_INT_ARR:
                fprintf(out, "global %s = <intarr:%ld>\n", inst->name ? inst->name : "<anon>", inst->imm);
                break;
            case IR_GLOBAL_STR:
                fprintf(out, "global %s = <str:%zu>\n", inst->name ? inst->name : "<anon>", inst->len);
                break;
            case IR_WRITE:
                fprintf(out, "t%d = write t%d, %zu\n", inst->dst, inst->lhs, inst->len);
                break;
            case IR_MOV:
                fprintf(out, "t%d = mov t%d\n", inst->dst, inst->lhs);
                break;
            case IR_FUNC:
                fprintf(out, "func %s\n", inst->name ? inst->name : "<anon>");
                break;
            case IR_PARAM:
                fprintf(out, "t%d = param %ld\n", inst->dst, inst->imm);
                break;
            case IR_CALL: {
                fprintf(out, "t%d = call %s(", inst->dst, inst->name ? inst->name : "<anon>");
                for (int i = 0; i < inst->argc; i++) {
                    if (i) fprintf(out, ", ");
                    fprintf(out, "t%d", inst->args[i]);
                }
                fprintf(out, ")\n");
                break;
            }
            case IR_RET:
                fprintf(out, "return t%d\n", inst->lhs);
                break;
            case IR_LABEL:
                fprintf(out, ".L%d:\n", inst->label);
                break;
            case IR_JMP:
                fprintf(out, "jmp .L%d\n", inst->label);
                break;
            case IR_BZ:
                fprintf(out, "bz t%d, .L%d\n", inst->lhs, inst->label);
                break;
            default:
                break;
        }
    }
}
