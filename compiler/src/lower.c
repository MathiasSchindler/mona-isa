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

typedef struct {
    const char *name;
    int temp;
} LocalBinding;

typedef struct {
    LocalBinding *items;
    size_t count;
    size_t cap;
} LocalMap;

static void locals_free(LocalMap *map) {
    free(map->items);
    map->items = NULL;
    map->count = 0;
    map->cap = 0;
}

static int locals_find(const LocalMap *map, const char *name) {
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int locals_add(LocalMap *map, const char *name, int temp) {
    if (map->count + 1 > map->cap) {
        size_t next = (map->cap == 0) ? 8 : map->cap * 2;
        LocalBinding *mem = (LocalBinding *)realloc(map->items, next * sizeof(LocalBinding));
        if (!mem) return 0;
        map->items = mem;
        map->cap = next;
    }
    map->items[map->count].name = name;
    map->items[map->count].temp = temp;
    map->count++;
    return 1;
}

static int lower_expr(const Expr *expr, IRProgram *ir, LocalMap *locals, char **out_error) {
    if (!expr) return -1;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return ir_emit_const(ir, expr->as.number);
        case EXPR_IDENT:
            if (locals) {
                int idx = locals_find(locals, expr->as.ident);
                if (idx >= 0) return locals->items[idx].temp;
            }
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unknown identifier");
            return -1;
        case EXPR_UNARY: {
            if (expr->as.unary.op == UN_PLUS) {
                return lower_expr(expr->as.unary.expr, ir, locals, out_error);
            }
            int rhs = lower_expr(expr->as.unary.expr, ir, locals, out_error);
            if (rhs < 0) return -1;
            int zero = ir_emit_const(ir, 0);
            if (zero < 0) return -1;
            return ir_emit_bin(ir, BIN_SUB, zero, rhs);
        }
        case EXPR_BINARY: {
            int lhs = lower_expr(expr->as.bin.left, ir, locals, out_error);
            if (lhs < 0) return -1;
            int rhs = lower_expr(expr->as.bin.right, ir, locals, out_error);
            if (rhs < 0) return -1;
            return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
        }
        case EXPR_CALL: {
            if (expr->as.call.arg_count > 8) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "too many call arguments");
                return -1;
            }
            int args[8];
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                int t = lower_expr(expr->as.call.args[i], ir, locals, out_error);
                if (t < 0) return -1;
                args[i] = t;
            }
            int temp = ir_emit_call(ir, expr->as.call.name, args, (int)expr->as.call.arg_count);
            if (temp < 0) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "out of memory");
                return -1;
            }
            return temp;
        }
        default:
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unsupported expression");
            return -1;
        }
    return -1;
}

static int lower_stmt(const Stmt *stmt, IRProgram *ir, LocalMap *locals, char **out_error);

static int lower_stmt_list(Stmt **stmts, size_t count, IRProgram *ir, LocalMap *locals, char **out_error) {
    for (size_t i = 0; i < count; i++) {
        if (!lower_stmt(stmts[i], ir, locals, out_error)) return 0;
    }
    return 1;
}

static int lower_stmt(const Stmt *stmt, IRProgram *ir, LocalMap *locals, char **out_error) {
    switch (stmt->kind) {
        case STMT_DECL: {
            if (locals_find(locals, stmt->name) >= 0) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: redeclared identifier");
                return 0;
            }
            int var_temp = ir_new_temp(ir);
            int expr_temp = -1;
            if (stmt->expr) {
                expr_temp = lower_expr(stmt->expr, ir, locals, out_error);
                if (expr_temp < 0) return 0;
            } else {
                expr_temp = ir_emit_const(ir, 0);
                if (expr_temp < 0) return 0;
            }
            ir_emit_mov(ir, var_temp, expr_temp);
            if (!locals_add(locals, stmt->name, var_temp)) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
                return 0;
            }
            return 1;
        }
        case STMT_ASSIGN: {
            int idx = locals_find(locals, stmt->name);
            if (idx < 0) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: unknown identifier");
                return 0;
            }
            int expr_temp = lower_expr(stmt->expr, ir, locals, out_error);
            if (expr_temp < 0) return 0;
            ir_emit_mov(ir, locals->items[idx].temp, expr_temp);
            return 1;
        }
        case STMT_RETURN: {
            int temp = lower_expr(stmt->expr, ir, locals, out_error);
            if (temp < 0) return 0;
            ir_emit_ret(ir, temp);
            return 1;
        }
        case STMT_BLOCK:
            return lower_stmt_list(stmt->stmts, stmt->stmt_count, ir, locals, out_error);
        case STMT_IF: {
            int cond_temp = lower_expr(stmt->cond, ir, locals, out_error);
            if (cond_temp < 0) return 0;
            int else_label = ir_new_label(ir);
            int end_label = ir_new_label(ir);
            ir_emit_bz(ir, cond_temp, else_label);
            if (!lower_stmt(stmt->then_branch, ir, locals, out_error)) return 0;
            if (stmt->else_branch) ir_emit_jmp(ir, end_label);
            ir_emit_label(ir, else_label);
            if (stmt->else_branch) {
                if (!lower_stmt(stmt->else_branch, ir, locals, out_error)) return 0;
                ir_emit_label(ir, end_label);
            }
            return 1;
        }
        case STMT_WHILE: {
            int start_label = ir_new_label(ir);
            int end_label = ir_new_label(ir);
            ir_emit_label(ir, start_label);
            int cond_temp = lower_expr(stmt->cond, ir, locals, out_error);
            if (cond_temp < 0) return 0;
            ir_emit_bz(ir, cond_temp, end_label);
            if (!lower_stmt(stmt->body, ir, locals, out_error)) return 0;
            ir_emit_jmp(ir, start_label);
            ir_emit_label(ir, end_label);
            return 1;
        }
        default:
            if (out_error && !*out_error) *out_error = dup_error_simple("error: unsupported statement");
            return 0;
    }
}

int lower_program(const Program *program, IRProgram *ir, char **out_error) {
    if (out_error) *out_error = NULL;
    if (!program || !program->funcs || program->func_count == 0) {
        if (out_error) *out_error = dup_error_simple("error: invalid program");
        return 0;
    }
    for (size_t f = 0; f < program->func_count; f++) {
        Function *fn = program->funcs[f];
        ir->temp_count = 0;
        ir->label_count = 0;
        ir_emit_func(ir, fn->name);
        LocalMap locals = {0};
        for (size_t i = 0; i < fn->param_count; i++) {
            int temp = ir_new_temp(ir);
            ir_emit_param(ir, (int)i, temp);
            if (!locals_add(&locals, fn->params[i], temp)) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
                locals_free(&locals);
                return 0;
            }
        }
        int ok = lower_stmt_list(fn->stmts, fn->stmt_count, ir, &locals, out_error);
        locals_free(&locals);
        if (!ok) return 0;
    }
    return 1;
}
