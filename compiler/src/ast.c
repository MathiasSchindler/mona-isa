#include "ast.h"
#include <stdlib.h>

static void free_expr(Expr *expr) {
    if (!expr) return;
    switch (expr->kind) {
        case EXPR_IDENT:
            free(expr->as.ident);
            break;
        case EXPR_BINARY:
            free_expr(expr->as.bin.left);
            free_expr(expr->as.bin.right);
            break;
        case EXPR_UNARY:
            free_expr(expr->as.unary.expr);
            break;
        case EXPR_NUMBER:
        default:
            break;
    }
    free(expr);
}

static void free_stmt(Stmt *stmt) {
    if (!stmt) return;
    if (stmt->kind == STMT_RETURN) free_expr(stmt->expr);
    free(stmt);
}

static void free_function(Function *func) {
    if (!func) return;
    free(func->name);
    free_stmt(func->body);
    free(func);
}

void free_program(Program *program) {
    if (!program) return;
    free_function(program->func);
    free(program);
}
