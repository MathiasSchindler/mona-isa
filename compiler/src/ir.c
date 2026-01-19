#include "ir.h"
#include <stdlib.h>

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
}

void ir_free(IRProgram *ir) {
    free(ir->insts);
    ir->insts = NULL;
    ir->count = 0;
    ir->cap = 0;
    ir->temp_count = 0;
}

int ir_emit_const(IRProgram *ir, long value) {
    int temp = ir->temp_count++;
    IRInst inst = {0};
    inst.op = IR_CONST;
    inst.dst = temp;
    inst.imm = value;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

int ir_emit_bin(IRProgram *ir, BinOp op, int lhs, int rhs) {
    int temp = ir->temp_count++;
    IRInst inst = {0};
    inst.op = IR_BIN;
    inst.dst = temp;
    inst.lhs = lhs;
    inst.rhs = rhs;
    inst.bin_op = op;
    if (!ir_push(ir, inst)) return -1;
    return temp;
}

void ir_emit_ret(IRProgram *ir, int value_temp) {
    IRInst inst = {0};
    inst.op = IR_RET;
    inst.lhs = value_temp;
    ir_push(ir, inst);
}

static const char *binop_name(BinOp op) {
    switch (op) {
        case BIN_ADD: return "add";
        case BIN_SUB: return "sub";
        case BIN_MUL: return "mul";
        case BIN_DIV: return "div";
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
            case IR_RET:
                fprintf(out, "return t%d\n", inst->lhs);
                break;
            default:
                break;
        }
    }
}
