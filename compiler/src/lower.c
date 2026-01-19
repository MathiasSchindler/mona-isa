#include "lower.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_error_at(const Expr *expr, const char *msg) {
    char buf[256];
    snprintf(buf, sizeof(buf), "error: %d:%d: %s", expr->line, expr->col, msg);
    size_t len = strlen(buf) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, buf, len);
    return out;
}

static char *dup_error_simple(const char *msg) {
    size_t len = strlen(msg) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, msg, len);
    return out;
}

static int lower_expr(const Expr *expr, IRProgram *ir, char **out_error) {
    if (!expr) return -1;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return ir_emit_const(ir, expr->as.number);
        case EXPR_IDENT:
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unknown identifier");
            return -1;
        case EXPR_UNARY: {
            if (expr->as.unary.op == UN_PLUS) {
                return lower_expr(expr->as.unary.expr, ir, out_error);
            }
            int rhs = lower_expr(expr->as.unary.expr, ir, out_error);
            if (rhs < 0) return -1;
            int zero = ir_emit_const(ir, 0);
            if (zero < 0) return -1;
            return ir_emit_bin(ir, BIN_SUB, zero, rhs);
        }
        case EXPR_BINARY: {
            int lhs = lower_expr(expr->as.bin.left, ir, out_error);
            if (lhs < 0) return -1;
            int rhs = lower_expr(expr->as.bin.right, ir, out_error);
            if (rhs < 0) return -1;
            return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
        }
        default:
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unsupported expression");
            return -1;
    }
}

int lower_program(const Program *program, IRProgram *ir, char **out_error) {
    if (out_error) *out_error = NULL;
    if (!program || !program->func || !program->func->body) {
        if (out_error) *out_error = dup_error_simple("error: invalid program");
        return 0;
    }
    const Stmt *stmt = program->func->body;
    if (stmt->kind != STMT_RETURN) {
        if (out_error) *out_error = dup_error_simple("error: unsupported statement");
        return 0;
    }
    int temp = lower_expr(stmt->expr, ir, out_error);
    if (temp < 0) return 0;
    ir_emit_ret(ir, temp);
    return 1;
}
