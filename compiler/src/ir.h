#ifndef MINAC_IR_H
#define MINAC_IR_H

#include <stddef.h>
#include <stdio.h>
#include "ast.h"

typedef enum {
    IR_CONST,
    IR_BIN,
    IR_RET
} IROp;

typedef struct {
    IROp op;
    int dst;
    long imm;
    int lhs;
    int rhs;
    BinOp bin_op;
} IRInst;

typedef struct {
    IRInst *insts;
    size_t count;
    size_t cap;
    int temp_count;
} IRProgram;

void ir_init(IRProgram *ir);
void ir_free(IRProgram *ir);
int ir_emit_const(IRProgram *ir, long value);
int ir_emit_bin(IRProgram *ir, BinOp op, int lhs, int rhs);
void ir_emit_ret(IRProgram *ir, int value_temp);
void ir_print(const IRProgram *ir, FILE *out);

#endif
