#ifndef MINAC_AST_H
#define MINAC_AST_H

#include <stddef.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL
} ExprKind;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_EQ,
    BIN_NEQ,
    BIN_LT,
    BIN_LTE,
    BIN_GT,
    BIN_GTE
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

typedef struct {
    char *name;
    Expr **args;
    size_t arg_count;
} ExprCall;

struct Expr {
    ExprKind kind;
    int line;
    int col;
    union {
        long number;
        char *ident;
        ExprBinary bin;
        ExprUnary unary;
        ExprCall call;
    } as;
};

typedef enum {
    STMT_RETURN,
    STMT_DECL,
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_BLOCK
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    char *name;
    Expr *expr;
    Expr *cond;
    struct Stmt *then_branch;
    struct Stmt *else_branch;
    struct Stmt *body;
    struct Stmt **stmts;
    size_t stmt_count;
} Stmt;

typedef struct Function {
    char *name;
    char **params;
    size_t param_count;
    Stmt **stmts;
    size_t stmt_count;
} Function;

typedef struct Program {
    Function **funcs;
    size_t func_count;
} Program;

void free_program(Program *program);

#endif
