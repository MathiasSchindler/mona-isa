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
    EXPR_INDEX,
    EXPR_SIZEOF,
    EXPR_FIELD
} ExprKind;

typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNION
} TypeKind;

typedef struct StructField {
    char *name;
    struct Type *type;
    size_t offset;
    size_t size;
} StructField;

typedef struct StructDef {
    char *name;
    int is_union;
    StructField *fields;
    size_t field_count;
    size_t size;
    size_t align;
} StructDef;

typedef struct Type {
    TypeKind kind;
    struct Type *base;
    size_t array_len;
    StructDef *def;
} Type;

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

typedef struct {
    Expr *base;
    char *field;
    int is_arrow;
} ExprField;

typedef struct {
    Type *type;
    Expr *expr;
} ExprSizeof;

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
        ExprSizeof sizeof_expr;
        ExprField field;
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
    Expr **init_list;
    size_t init_count;
    Expr *lhs;
    Expr *index;
    Expr *cond;
    Type *decl_type;
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
    Type **param_types;
    size_t param_count;
    Stmt **stmts;
    size_t stmt_count;
    Type *ret_type;
} Function;

typedef enum {
    GLOB_INT,
    GLOB_CHAR,
    GLOB_STR,
    GLOB_INT_ARR,
    GLOB_BYTES
} GlobalKind;

typedef struct {
    GlobalKind kind;
    char *name;
    long value;
    char *data;
    size_t len;
    Type *type;
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
    StructDef **structs;
    size_t struct_count;
} Program;

void free_program(Program *program);

#endif
