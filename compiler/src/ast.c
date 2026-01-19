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
        case EXPR_CALL:
            free(expr->as.call.name);
            for (size_t i = 0; i < expr->as.call.arg_count; i++) free_expr(expr->as.call.args[i]);
            free(expr->as.call.args);
            break;
        case EXPR_NUMBER:
        default:
            break;
    }
    free(expr);
}

static void free_stmt(Stmt *stmt) {
    if (!stmt) return;
    if (stmt->kind == STMT_DECL || stmt->kind == STMT_ASSIGN) free(stmt->name);
    if (stmt->expr) free_expr(stmt->expr);
    if (stmt->cond) free_expr(stmt->cond);
    if (stmt->then_branch) free_stmt(stmt->then_branch);
    if (stmt->else_branch) free_stmt(stmt->else_branch);
    if (stmt->body) free_stmt(stmt->body);
    if (stmt->stmts) {
        for (size_t i = 0; i < stmt->stmt_count; i++) free_stmt(stmt->stmts[i]);
        free(stmt->stmts);
    }
    free(stmt);
}

static void free_function(Function *func) {
    if (!func) return;
    free(func->name);
    for (size_t i = 0; i < func->param_count; i++) free(func->params[i]);
    free(func->params);
    for (size_t i = 0; i < func->stmt_count; i++) {
        free_stmt(func->stmts[i]);
    }
    free(func->stmts);
    free(func);
}

void free_program(Program *program) {
    if (!program) return;
    for (size_t i = 0; i < program->func_count; i++) {
        free_function(program->funcs[i]);
    }
    free(program->funcs);
    free(program);
}
