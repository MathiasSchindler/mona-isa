#ifndef MINAC_IR_H
#define MINAC_IR_H

#include <stddef.h>
#include <stdio.h>
#include "ast.h"

typedef enum {
    IR_CONST,
    IR_BIN,
    IR_MOV,
    IR_ADDR,
    IR_LOAD,
    IR_STORE,
    IR_GLOBAL_INT,
    IR_GLOBAL_INT_ARR,
    IR_GLOBAL_STR,
    IR_WRITE,
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
    char *data;
    size_t len;
    long *values;
    size_t val_count;
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
int ir_emit_addr(IRProgram *ir, const char *name);
int ir_emit_load(IRProgram *ir, int addr_temp);
void ir_emit_store(IRProgram *ir, int addr_temp, int value_temp);
void ir_emit_global_int(IRProgram *ir, const char *name, long value);
void ir_emit_global_int_arr(IRProgram *ir, const char *name, const long *values, size_t count, size_t init_count);
void ir_emit_global_str(IRProgram *ir, const char *name, const char *data, size_t len);
int ir_emit_write(IRProgram *ir, int addr_temp, size_t len);
void ir_emit_func(IRProgram *ir, const char *name);
void ir_emit_param(IRProgram *ir, int index, int dst);
int ir_emit_call(IRProgram *ir, const char *name, const int *args, int argc);
int ir_new_label(IRProgram *ir);
void ir_emit_label(IRProgram *ir, int label);
void ir_emit_jmp(IRProgram *ir, int label);
void ir_emit_bz(IRProgram *ir, int cond_temp, int label);
void ir_print(const IRProgram *ir, FILE *out);

#endif
