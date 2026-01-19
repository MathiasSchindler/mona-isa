#ifndef MINAC_AST_H
#define MINAC_AST_H

#include <stddef.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_STRING,
    EXPR_INDEX
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
    BIN_GTE,
    BIN_LOGAND,
    BIN_LOGOR
} BinOp;

typedef enum {
    UN_PLUS,
    UN_MINUS,
    UN_NOT,
    UN_ADDR,
    UN_DEREF
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

typedef struct {
    char *data;
    size_t len;
} ExprString;

typedef struct {
    Expr *base;
    Expr *index;
} ExprIndex;

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
        ExprString str;
        ExprIndex index;
    } as;
};

typedef enum {
    STMT_RETURN,
    STMT_DECL,
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_IF,
    STMT_WHILE,
    STMT_BLOCK,
    STMT_FOR,
    STMT_SWITCH,
    STMT_BREAK,
    STMT_CONTINUE
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    char *name;
    Expr *expr;
    Expr *lhs;
    Expr *index;
    Expr *cond;
    struct Stmt *then_branch;
    struct Stmt *else_branch;
    struct Stmt *body;
    struct Stmt *init;
    struct Stmt *post;
    struct Stmt **stmts;
    size_t stmt_count;
    struct SwitchCase *cases;
    size_t case_count;
    struct Stmt **default_stmts;
    size_t default_count;
} Stmt;

typedef struct Function {
    char *name;
    char **params;
    size_t param_count;
    Stmt **stmts;
    size_t stmt_count;
} Function;

typedef enum {
    GLOB_INT,
    GLOB_STR,
    GLOB_INT_ARR
} GlobalKind;

typedef struct {
    GlobalKind kind;
    char *name;
    long value;
    char *data;
    size_t len;
} Global;

typedef struct SwitchCase {
    long value;
    struct Stmt **stmts;
    size_t stmt_count;
} SwitchCase;

typedef struct Program {
    Function **funcs;
    size_t func_count;
    Global **globals;
    size_t global_count;
} Program;

void free_program(Program *program);

#endif
