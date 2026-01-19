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
    int addressable;
} LocalBinding;

typedef struct {
    LocalBinding *items;
    size_t count;
    size_t cap;
} LocalMap;

typedef struct {
    const char *name;
    GlobalKind kind;
    size_t len;
} GlobalBinding;

typedef struct {
    GlobalBinding *items;
    size_t count;
    size_t cap;
    int str_count;
} GlobalMap;

typedef struct {
    int break_label;
    int continue_label;
} LoopCtx;

static void locals_free(LocalMap *map) {
    free(map->items);
    map->items = NULL;
    map->count = 0;
    map->cap = 0;
}

static void globals_free(GlobalMap *map) {
    free(map->items);
    map->items = NULL;
    map->count = 0;
    map->cap = 0;
    map->str_count = 0;
}

static int globals_find(const GlobalMap *map, const char *name) {
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int globals_add(GlobalMap *map, const char *name, GlobalKind kind, size_t len) {
    if (map->count + 1 > map->cap) {
        size_t next = (map->cap == 0) ? 8 : map->cap * 2;
        GlobalBinding *mem = (GlobalBinding *)realloc(map->items, next * sizeof(GlobalBinding));
        if (!mem) return 0;
        map->items = mem;
        map->cap = next;
    }
    map->items[map->count].name = name;
    map->items[map->count].kind = kind;
    map->items[map->count].len = len;
    map->count++;
    return 1;
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
    map->items[map->count].addressable = 0;
    map->count++;
    return 1;
}

static int locals_mark_addressable(LocalMap *map, const char *name) {
    if (!map || !name) return 0;
    int idx = locals_find(map, name);
    if (idx < 0) return 0;
    if (map->items[idx].addressable) return 0;
    map->items[idx].addressable = 1;
    return 1;
}

static int lower_addr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error);
static int lower_expr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error);

static int lower_index_addr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error) {
    if (!expr || expr->kind != EXPR_INDEX) return -1;
    int base_addr = -1;
    if (expr->as.index.base->kind == EXPR_IDENT) {
        base_addr = lower_addr(expr->as.index.base, ir, locals, globals, out_error);
    } else {
        base_addr = lower_expr(expr->as.index.base, ir, locals, globals, out_error);
    }
    if (base_addr < 0) return -1;
    int idx = lower_expr(expr->as.index.index, ir, locals, globals, out_error);
    if (idx < 0) return -1;
    int scale = ir_emit_const(ir, 8);
    if (scale < 0) return -1;
    int scaled = ir_emit_bin(ir, BIN_MUL, idx, scale);
    if (scaled < 0) return -1;
    return ir_emit_bin(ir, BIN_ADD, base_addr, scaled);
}

static int lower_addr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error) {
    if (!expr) return -1;
    if (expr->kind == EXPR_IDENT) {
        if (locals) {
            int idx = locals_find(locals, expr->as.ident);
            if (idx >= 0) {
                int newly = locals_mark_addressable(locals, expr->as.ident);
                int addr = ir_emit_addr(ir, expr->as.ident);
                if (addr < 0) return -1;
                if (newly && locals->items[idx].temp >= 0) {
                    ir_emit_store(ir, addr, locals->items[idx].temp);
                    locals->items[idx].temp = -1;
                }
                return addr;
            }
        }
        if (globals) {
            int gidx = globals_find(globals, expr->as.ident);
            if (gidx >= 0) return ir_emit_addr(ir, expr->as.ident);
        }
        if (out_error && !*out_error) *out_error = dup_error_at(expr, "unknown identifier");
        return -1;
    }
    if (expr->kind == EXPR_INDEX) return lower_index_addr(expr, ir, locals, globals, out_error);
    if (expr->kind == EXPR_UNARY && expr->as.unary.op == UN_DEREF) {
        return lower_expr(expr->as.unary.expr, ir, locals, globals, out_error);
    }
    if (out_error && !*out_error) *out_error = dup_error_at(expr, "invalid lvalue");
    return -1;
}

static int lower_expr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error) {
    if (!expr) return -1;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return ir_emit_const(ir, expr->as.number);
        case EXPR_IDENT:
            if (locals) {
                int idx = locals_find(locals, expr->as.ident);
                if (idx >= 0) {
                    if (locals->items[idx].addressable) {
                        int addr = ir_emit_addr(ir, expr->as.ident);
                        if (addr < 0) return -1;
                        return ir_emit_load(ir, addr);
                    }
                    return locals->items[idx].temp;
                }
            }
            if (globals) {
                int gidx = globals_find(globals, expr->as.ident);
                if (gidx >= 0) {
                    if (globals->items[gidx].kind == GLOB_STR || globals->items[gidx].kind == GLOB_INT_ARR) {
                        return ir_emit_addr(ir, expr->as.ident);
                    }
                    int addr = ir_emit_addr(ir, expr->as.ident);
                    if (addr < 0) return -1;
                    return ir_emit_load(ir, addr);
                }
            }
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unknown identifier");
            return -1;
        case EXPR_UNARY: {
            if (expr->as.unary.op == UN_PLUS) {
                return lower_expr(expr->as.unary.expr, ir, locals, globals, out_error);
            }
            if (expr->as.unary.op == UN_NOT) {
                int rhs = lower_expr(expr->as.unary.expr, ir, locals, globals, out_error);
                if (rhs < 0) return -1;
                int zero = ir_emit_const(ir, 0);
                if (zero < 0) return -1;
                return ir_emit_bin(ir, BIN_EQ, rhs, zero);
            }
            if (expr->as.unary.op == UN_ADDR) {
                return lower_addr(expr->as.unary.expr, ir, locals, globals, out_error);
            }
            if (expr->as.unary.op == UN_DEREF) {
                int addr = lower_expr(expr->as.unary.expr, ir, locals, globals, out_error);
                if (addr < 0) return -1;
                return ir_emit_load(ir, addr);
            }
            int rhs = lower_expr(expr->as.unary.expr, ir, locals, globals, out_error);
            if (rhs < 0) return -1;
            int zero = ir_emit_const(ir, 0);
            if (zero < 0) return -1;
            return ir_emit_bin(ir, BIN_SUB, zero, rhs);
        }
        case EXPR_BINARY: {
            if (expr->as.bin.op == BIN_LOGAND) {
                int result = ir_new_temp(ir);
                int false_label = ir_new_label(ir);
                int end_label = ir_new_label(ir);
                int lhs = lower_expr(expr->as.bin.left, ir, locals, globals, out_error);
                if (lhs < 0) return -1;
                ir_emit_bz(ir, lhs, false_label);
                int rhs = lower_expr(expr->as.bin.right, ir, locals, globals, out_error);
                if (rhs < 0) return -1;
                ir_emit_bz(ir, rhs, false_label);
                int one = ir_emit_const(ir, 1);
                if (one < 0) return -1;
                ir_emit_mov(ir, result, one);
                ir_emit_jmp(ir, end_label);
                ir_emit_label(ir, false_label);
                int zero = ir_emit_const(ir, 0);
                if (zero < 0) return -1;
                ir_emit_mov(ir, result, zero);
                ir_emit_label(ir, end_label);
                return result;
            }
            if (expr->as.bin.op == BIN_LOGOR) {
                int result = ir_new_temp(ir);
                int rhs_label = ir_new_label(ir);
                int false_label = ir_new_label(ir);
                int end_label = ir_new_label(ir);
                int lhs = lower_expr(expr->as.bin.left, ir, locals, globals, out_error);
                if (lhs < 0) return -1;
                ir_emit_bz(ir, lhs, rhs_label);
                int one = ir_emit_const(ir, 1);
                if (one < 0) return -1;
                ir_emit_mov(ir, result, one);
                ir_emit_jmp(ir, end_label);
                ir_emit_label(ir, rhs_label);
                int rhs = lower_expr(expr->as.bin.right, ir, locals, globals, out_error);
                if (rhs < 0) return -1;
                ir_emit_bz(ir, rhs, false_label);
                int one2 = ir_emit_const(ir, 1);
                if (one2 < 0) return -1;
                ir_emit_mov(ir, result, one2);
                ir_emit_jmp(ir, end_label);
                ir_emit_label(ir, false_label);
                int zero = ir_emit_const(ir, 0);
                if (zero < 0) return -1;
                ir_emit_mov(ir, result, zero);
                ir_emit_label(ir, end_label);
                return result;
            }
            int lhs = lower_expr(expr->as.bin.left, ir, locals, globals, out_error);
            if (lhs < 0) return -1;
            int rhs = lower_expr(expr->as.bin.right, ir, locals, globals, out_error);
            if (rhs < 0) return -1;
            return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
        }
        case EXPR_CALL: {
            if (expr->as.call.arg_count == 1 && strcmp(expr->as.call.name, "puts") == 0) {
                const Expr *arg = expr->as.call.args[0];
                int addr_temp = -1;
                size_t len = 0;
                if (arg->kind == EXPR_STRING) {
                    char label[32];
                    snprintf(label, sizeof(label), ".Lstr%d", globals ? globals->str_count++ : 0);
                    ir_emit_global_str(ir, label, arg->as.str.data, arg->as.str.len);
                    addr_temp = ir_emit_addr(ir, label);
                    len = arg->as.str.len;
                } else if (arg->kind == EXPR_IDENT && globals) {
                    int gidx = globals_find(globals, arg->as.ident);
                    if (gidx >= 0 && globals->items[gidx].kind == GLOB_STR) {
                        addr_temp = ir_emit_addr(ir, arg->as.ident);
                        len = globals->items[gidx].len;
                    }
                }
                if (addr_temp < 0) {
                    if (out_error && !*out_error) *out_error = dup_error_at(expr, "puts expects a string literal or global string");
                    return -1;
                }
                return ir_emit_write(ir, addr_temp, len);
            }
            if (expr->as.call.arg_count > 8) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "too many call arguments");
                return -1;
            }
            int args[8];
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                int t = lower_expr(expr->as.call.args[i], ir, locals, globals, out_error);
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
        case EXPR_STRING: {
            char label[32];
            snprintf(label, sizeof(label), ".Lstr%d", globals ? globals->str_count++ : 0);
            ir_emit_global_str(ir, label, expr->as.str.data, expr->as.str.len);
            return ir_emit_addr(ir, label);
        }
        case EXPR_INDEX: {
            int addr = lower_index_addr(expr, ir, locals, globals, out_error);
            if (addr < 0) return -1;
            return ir_emit_load(ir, addr);
        }
        default:
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unsupported expression");
            return -1;
        }
    return -1;
}

static int lower_stmt(const Stmt *stmt, IRProgram *ir, LocalMap *locals, GlobalMap *globals, LoopCtx *ctx, char **out_error);

static int lower_stmt_list(Stmt **stmts, size_t count, IRProgram *ir, LocalMap *locals, GlobalMap *globals, LoopCtx *ctx, char **out_error) {
    for (size_t i = 0; i < count; i++) {
        if (!lower_stmt(stmts[i], ir, locals, globals, ctx, out_error)) return 0;
    }
    return 1;
}

static int lower_stmt(const Stmt *stmt, IRProgram *ir, LocalMap *locals, GlobalMap *globals, LoopCtx *ctx, char **out_error) {
    switch (stmt->kind) {
        case STMT_DECL: {
            if (locals_find(locals, stmt->name) >= 0) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: redeclared identifier");
                return 0;
            }
            int var_temp = ir_new_temp(ir);
            int expr_temp = -1;
            if (stmt->expr) {
                expr_temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
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
            if (stmt->lhs) {
                if (stmt->lhs->kind == EXPR_IDENT) {
                    int idx = locals_find(locals, stmt->lhs->as.ident);
                    if (idx >= 0 && !locals->items[idx].addressable) {
                        int expr_temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
                        if (expr_temp < 0) return 0;
                        ir_emit_mov(ir, locals->items[idx].temp, expr_temp);
                        return 1;
                    }
                }
                int addr = lower_addr(stmt->lhs, ir, locals, globals, out_error);
                if (addr < 0) return 0;
                int expr_temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
                if (expr_temp < 0) return 0;
                ir_emit_store(ir, addr, expr_temp);
                return 1;
            }
            if (stmt->name) {
                int idx = locals_find(locals, stmt->name);
                if (idx >= 0 && !locals->items[idx].addressable) {
                    int expr_temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
                    if (expr_temp < 0) return 0;
                    ir_emit_mov(ir, locals->items[idx].temp, expr_temp);
                    return 1;
                }
                if (globals) {
                    int gidx = globals_find(globals, stmt->name);
                    if (gidx >= 0) {
                        int addr = ir_emit_addr(ir, stmt->name);
                        if (addr < 0) return 0;
                        int expr_temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
                        if (expr_temp < 0) return 0;
                        ir_emit_store(ir, addr, expr_temp);
                        return 1;
                    }
                }
                if (out_error && !*out_error) *out_error = dup_error_simple("error: unknown identifier");
                return 0;
            }
            if (out_error && !*out_error) *out_error = dup_error_simple("error: invalid assignment");
            return 0;
        }
        case STMT_RETURN: {
            int temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
            if (temp < 0) return 0;
            ir_emit_ret(ir, temp);
            return 1;
        }
        case STMT_EXPR: {
            int temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
            return temp >= 0;
        }
        case STMT_BLOCK:
            return lower_stmt_list(stmt->stmts, stmt->stmt_count, ir, locals, globals, ctx, out_error);
        case STMT_IF: {
            int cond_temp = lower_expr(stmt->cond, ir, locals, globals, out_error);
            if (cond_temp < 0) return 0;
            int else_label = ir_new_label(ir);
            int end_label = ir_new_label(ir);
            ir_emit_bz(ir, cond_temp, else_label);
            if (!lower_stmt(stmt->then_branch, ir, locals, globals, ctx, out_error)) return 0;
            if (stmt->else_branch) ir_emit_jmp(ir, end_label);
            ir_emit_label(ir, else_label);
            if (stmt->else_branch) {
                if (!lower_stmt(stmt->else_branch, ir, locals, globals, ctx, out_error)) return 0;
                ir_emit_label(ir, end_label);
            }
            return 1;
        }
        case STMT_WHILE: {
            int start_label = ir_new_label(ir);
            int end_label = ir_new_label(ir);
            ir_emit_label(ir, start_label);
            int cond_temp = lower_expr(stmt->cond, ir, locals, globals, out_error);
            if (cond_temp < 0) return 0;
            ir_emit_bz(ir, cond_temp, end_label);
            LoopCtx loop_ctx = { end_label, start_label };
            if (!lower_stmt(stmt->body, ir, locals, globals, &loop_ctx, out_error)) return 0;
            ir_emit_jmp(ir, start_label);
            ir_emit_label(ir, end_label);
            return 1;
        }
        case STMT_FOR: {
            if (stmt->init) {
                if (!lower_stmt(stmt->init, ir, locals, globals, ctx, out_error)) return 0;
            }
            int start_label = ir_new_label(ir);
            int post_label = ir_new_label(ir);
            int end_label = ir_new_label(ir);
            ir_emit_label(ir, start_label);
            if (stmt->cond) {
                int cond_temp = lower_expr(stmt->cond, ir, locals, globals, out_error);
                if (cond_temp < 0) return 0;
                ir_emit_bz(ir, cond_temp, end_label);
            }
            LoopCtx loop_ctx = { end_label, post_label };
            if (!lower_stmt(stmt->body, ir, locals, globals, &loop_ctx, out_error)) return 0;
            ir_emit_label(ir, post_label);
            if (stmt->post) {
                if (!lower_stmt(stmt->post, ir, locals, globals, ctx, out_error)) return 0;
            }
            ir_emit_jmp(ir, start_label);
            ir_emit_label(ir, end_label);
            return 1;
        }
        case STMT_SWITCH: {
            int end_label = ir_new_label(ir);
            int cond_temp = lower_expr(stmt->cond, ir, locals, globals, out_error);
            if (cond_temp < 0) return 0;

            int *case_labels = NULL;
            int *next_labels = NULL;
            if (stmt->case_count > 0) {
                case_labels = (int *)malloc(stmt->case_count * sizeof(int));
                next_labels = (int *)malloc(stmt->case_count * sizeof(int));
                if (!case_labels || !next_labels) { free(case_labels); free(next_labels); if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory"); return 0; }
                for (size_t i = 0; i < stmt->case_count; i++) {
                    case_labels[i] = ir_new_label(ir);
                    next_labels[i] = ir_new_label(ir);
                }
            }
            int default_label = (stmt->default_count > 0) ? ir_new_label(ir) : end_label;

            for (size_t i = 0; i < stmt->case_count; i++) {
                int val = ir_emit_const(ir, stmt->cases[i].value);
                if (val < 0) { free(case_labels); free(next_labels); return 0; }
                int eq = ir_emit_bin(ir, BIN_EQ, cond_temp, val);
                if (eq < 0) { free(case_labels); free(next_labels); return 0; }
                ir_emit_bz(ir, eq, next_labels[i]);
                ir_emit_jmp(ir, case_labels[i]);
                ir_emit_label(ir, next_labels[i]);
            }
            ir_emit_jmp(ir, default_label);

            LoopCtx switch_ctx = { end_label, ctx ? ctx->continue_label : -1 };
            for (size_t i = 0; i < stmt->case_count; i++) {
                ir_emit_label(ir, case_labels[i]);
                if (!lower_stmt_list(stmt->cases[i].stmts, stmt->cases[i].stmt_count, ir, locals, globals, &switch_ctx, out_error)) { free(case_labels); free(next_labels); return 0; }
            }
            if (stmt->default_count > 0) {
                ir_emit_label(ir, default_label);
                if (!lower_stmt_list(stmt->default_stmts, stmt->default_count, ir, locals, globals, &switch_ctx, out_error)) { free(case_labels); free(next_labels); return 0; }
            }
            ir_emit_label(ir, end_label);
            free(case_labels);
            free(next_labels);
            return 1;
        }
        case STMT_BREAK:
            if (!ctx || ctx->break_label < 0) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: 'break' outside of loop/switch");
                return 0;
            }
            ir_emit_jmp(ir, ctx->break_label);
            return 1;
        case STMT_CONTINUE:
            if (!ctx || ctx->continue_label < 0) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: 'continue' outside of loop");
                return 0;
            }
            ir_emit_jmp(ir, ctx->continue_label);
            return 1;
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
    GlobalMap globals = {0};
    for (size_t i = 0; i < program->global_count; i++) {
        Global *g = program->globals[i];
        if (!g) continue;
        if (g->kind == GLOB_INT) ir_emit_global_int(ir, g->name, g->value);
        if (g->kind == GLOB_INT_ARR) {
            const long *vals = (const long *)g->data;
            ir_emit_global_int_arr(ir, g->name, vals, (size_t)g->value, g->len);
        }
        if (g->kind == GLOB_STR) ir_emit_global_str(ir, g->name, g->data, g->len);
        if (!globals_add(&globals, g->name, g->kind, g->len)) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
            globals_free(&globals);
            return 0;
        }
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
                globals_free(&globals);
                return 0;
            }
        }
        LoopCtx root_ctx = { -1, -1 };
        int ok = lower_stmt_list(fn->stmts, fn->stmt_count, ir, &locals, &globals, &root_ctx, out_error);
        locals_free(&locals);
        if (!ok) { globals_free(&globals); return 0; }
    }
    globals_free(&globals);
    return 1;
}
