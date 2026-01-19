#ifndef MINAC_IR_H
#define MINAC_IR_H

#include <stddef.h>
#include <stdio.h>
#include "ast.h"

typedef enum {
    IR_CONST,
    IR_BIN,
    IR_MOV,
    IR_FUNC,
    IR_PARAM,
    IR_CALL,
    IR_RET,
    IR_LABEL,
    IR_JMP,
    IR_BZ
} IROp;

typedef struct {
    IROp op;
    int dst;
    long imm;
    int lhs;
    int rhs;
    int label;
    BinOp bin_op;
    const char *name;
    int *args;
    int argc;
} IRInst;

typedef struct {
    IRInst *insts;
    size_t count;
    size_t cap;
    int temp_count;
    int label_count;
} IRProgram;

void ir_init(IRProgram *ir);
void ir_free(IRProgram *ir);
int ir_new_temp(IRProgram *ir);
int ir_emit_const(IRProgram *ir, long value);
int ir_emit_bin(IRProgram *ir, BinOp op, int lhs, int rhs);
void ir_emit_mov(IRProgram *ir, int dst, int src);
void ir_emit_ret(IRProgram *ir, int value_temp);
void ir_emit_func(IRProgram *ir, const char *name);
void ir_emit_param(IRProgram *ir, int index, int dst);
int ir_emit_call(IRProgram *ir, const char *name, const int *args, int argc);
int ir_new_label(IRProgram *ir);
void ir_emit_label(IRProgram *ir, int label);
void ir_emit_jmp(IRProgram *ir, int label);
void ir_emit_bz(IRProgram *ir, int cond_temp, int label);
void ir_print(const IRProgram *ir, FILE *out);

#endif
