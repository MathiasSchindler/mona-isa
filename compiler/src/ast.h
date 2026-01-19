#ifndef MINAC_AST_H
#define MINAC_AST_H

#include <stddef.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_UNARY
} ExprKind;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV
} BinOp;

typedef enum {
    UN_PLUS,
    UN_MINUS
} UnOp;

typedef struct Expr Expr;

typedef struct {
    BinOp op;
    Expr *left;
    Expr *right;
} ExprBinary;

typedef struct {
    UnOp op;
    Expr *expr;
} ExprUnary;

struct Expr {
    ExprKind kind;
    int line;
    int col;
    union {
        long number;
        char *ident;
        ExprBinary bin;
        ExprUnary unary;
    } as;
};

typedef enum {
    STMT_RETURN
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    Expr *expr;
} Stmt;

typedef struct Function {
    char *name;
    Stmt *body;
} Function;

typedef struct Program {
    Function *func;
} Program;

void free_program(Program *program);

#endif
