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

static char *dup_label(const char *prefix, int idx) {
    int needed = snprintf(NULL, 0, "%s%d", prefix, idx);
    if (needed < 0) return NULL;
    size_t len = (size_t)needed + 1;
    char *out = (char *)malloc(len);
    if (!out) return NULL;
    snprintf(out, len, "%s%d", prefix, idx);
    return out;
}

typedef struct {
    const char *name;
    int temp;
    int addressable;
    Type *type;
    size_t size;
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
    Type *type;
} GlobalBinding;

typedef struct {
    GlobalBinding *items;
    size_t count;
    size_t cap;
    int str_count;
    int byte_count;
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

static int globals_add(GlobalMap *map, const char *name, GlobalKind kind, size_t len, Type *type) {
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
    map->items[map->count].type = type;
    map->count++;
    return 1;
}

static int locals_find(const LocalMap *map, const char *name) {
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int locals_add(LocalMap *map, const char *name, int temp, Type *type, size_t size) {
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
    map->items[map->count].type = type;
    map->items[map->count].size = size;
    map->count++;
    return 1;
}

static int type_is_char(const Type *type) {
    return type && type->kind == TYPE_CHAR;
}

static int type_is_ptr(const Type *type) {
    return type && (type->kind == TYPE_PTR || type->kind == TYPE_ARRAY);
}

static const Type *type_base(const Type *type) {
    if (!type) return NULL;
    if (type->kind == TYPE_PTR || type->kind == TYPE_ARRAY) return type->base;
    return NULL;
}

static int type_size(const Type *type) {
    if (!type) return 8;
    if (type->kind == TYPE_CHAR) return 1;
    if (type->kind == TYPE_VOID) return 0;
    if (type->kind == TYPE_ARRAY) {
        int base = type_size(type->base);
        return (int)(base * (int)type->array_len);
    }
    if (type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) {
        return type->def ? (int)type->def->size : 0;
    }
    return 8;
}

static const StructField *find_field(const StructDef *def, const char *name) {
    if (!def || !name) return NULL;
    for (size_t i = 0; i < def->field_count; i++) {
        if (strcmp(def->fields[i].name, name) == 0) return &def->fields[i];
    }
    return NULL;
}

static Type *lookup_local_type(LocalMap *locals, const char *name) {
    if (!locals || !name) return NULL;
    int idx = locals_find(locals, name);
    if (idx < 0) return NULL;
    return locals->items[idx].type;
}

static Type *lookup_global_type(GlobalMap *globals, const char *name) {
    if (!globals || !name) return NULL;
    int idx = globals_find(globals, name);
    if (idx < 0) return NULL;
    return globals->items[idx].type;
}

static int emit_load_by_type(IRProgram *ir, int addr, const Type *type) {
    if (type_is_char(type)) return ir_emit_load8(ir, addr);
    return ir_emit_load(ir, addr);
}

static void emit_store_by_type(IRProgram *ir, int addr, int value, const Type *type) {
    if (type_is_char(type)) ir_emit_store8(ir, addr, value);
    else ir_emit_store(ir, addr, value);
}

static Type *expr_type(const Expr *expr, LocalMap *locals, GlobalMap *globals);

static const Type *lvalue_type(const Expr *expr, LocalMap *locals, GlobalMap *globals) {
    if (!expr) return NULL;
    if (expr->kind == EXPR_IDENT) {
        Type *lt = lookup_local_type(locals, expr->as.ident);
        if (lt) return (lt->kind == TYPE_ARRAY) ? lt->base : lt;
        return lookup_global_type(globals, expr->as.ident);
    }
    if (expr->kind == EXPR_INDEX) {
        if (expr->as.index.base->kind == EXPR_IDENT) {
            const char *name = expr->as.index.base->as.ident;
            Type *lt = lookup_local_type(locals, name);
            if (lt && type_is_ptr(lt)) return type_base(lt);
            Type *gt = lookup_global_type(globals, name);
            if (gt && type_is_ptr(gt)) return type_base(gt);
        }
        return NULL;
    }
    if (expr->kind == EXPR_UNARY && expr->as.unary.op == UN_DEREF) {
        const Expr *inner = expr->as.unary.expr;
        if (inner->kind == EXPR_IDENT) {
            Type *lt = lookup_local_type(locals, inner->as.ident);
            if (lt && type_is_ptr(lt)) return type_base(lt);
            Type *gt = lookup_global_type(globals, inner->as.ident);
            if (gt && type_is_ptr(gt)) return type_base(gt);
        }
        return NULL;
    }
    if (expr->kind == EXPR_FIELD) {
        Type *base = expr_type(expr->as.field.base, locals, globals);
        if (expr->as.field.is_arrow) {
            if (!type_is_ptr(base)) return NULL;
            base = (Type *)type_base(base);
        }
        if (!base || (base->kind != TYPE_STRUCT && base->kind != TYPE_UNION)) return NULL;
        const StructField *field = find_field(base->def, expr->as.field.field);
        if (!field) return NULL;
        return field->type;
    }
    return NULL;
}

static Type *expr_type(const Expr *expr, LocalMap *locals, GlobalMap *globals) {
    static Type t_int = { TYPE_INT, NULL, 0, NULL };
    static Type t_char = { TYPE_CHAR, NULL, 0, NULL };
    static Type t_char_ptr = { TYPE_PTR, &t_char, 0, NULL };
    if (!expr) return &t_int;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return &t_int;
        case EXPR_STRING:
            return &t_char_ptr;
        case EXPR_IDENT: {
            Type *lt = lookup_local_type(locals, expr->as.ident);
            if (lt) return lt;
            Type *gt = lookup_global_type(globals, expr->as.ident);
            if (gt) return gt;
            return &t_int;
        }
        case EXPR_INDEX:
        case EXPR_UNARY:
            return (Type *)lvalue_type(expr, locals, globals);
        case EXPR_FIELD:
            return (Type *)lvalue_type(expr, locals, globals);
        case EXPR_CALL:
            return &t_int;
        default:
            return &t_int;
    }
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
static int g_prefer_libc = 0;
static int lower_expr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error);

static int lower_index_addr(const Expr *expr, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error) {
    if (!expr || expr->kind != EXPR_INDEX) return -1;
    int base_addr = -1;
    if (expr->as.index.base->kind == EXPR_IDENT) {
        const char *name = expr->as.index.base->as.ident;
        Type *lt = lookup_local_type(locals, name);
        Type *gt = lookup_global_type(globals, name);
        Type *bt = lt ? lt : gt;
        if (bt && bt->kind == TYPE_ARRAY) {
            base_addr = lower_addr(expr->as.index.base, ir, locals, globals, out_error);
        } else {
            base_addr = lower_expr(expr->as.index.base, ir, locals, globals, out_error);
        }
    } else {
        base_addr = lower_expr(expr->as.index.base, ir, locals, globals, out_error);
    }
    if (base_addr < 0) return -1;
    int idx = lower_expr(expr->as.index.index, ir, locals, globals, out_error);
    if (idx < 0) return -1;
    int elem_size = 8;
    if (expr->as.index.base->kind == EXPR_IDENT) {
        const char *name = expr->as.index.base->as.ident;
        Type *lt = lookup_local_type(locals, name);
        if (lt && type_is_ptr(lt)) elem_size = type_size(type_base(lt));
        Type *gt = lookup_global_type(globals, name);
        if (gt && type_is_ptr(gt)) elem_size = type_size(type_base(gt));
    }
    int scale = ir_emit_const(ir, elem_size);
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
                if (newly) {
                    ir_emit_local_alloc(ir, expr->as.ident, locals->items[idx].size);
                    if (locals->items[idx].temp >= 0) {
                        emit_store_by_type(ir, addr, locals->items[idx].temp, locals->items[idx].type);
                        locals->items[idx].temp = -1;
                    }
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
    if (expr->kind == EXPR_FIELD) {
        Type *base_type = expr_type(expr->as.field.base, locals, globals);
        int base_addr = -1;
        if (expr->as.field.is_arrow) {
            if (!type_is_ptr(base_type)) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "invalid '->' base");
                return -1;
            }
            base_type = (Type *)type_base(base_type);
            base_addr = lower_expr(expr->as.field.base, ir, locals, globals, out_error);
        } else {
            base_addr = lower_addr(expr->as.field.base, ir, locals, globals, out_error);
        }
        if (base_addr < 0) return -1;
        if (!base_type || (base_type->kind != TYPE_STRUCT && base_type->kind != TYPE_UNION)) {
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "invalid field access");
            return -1;
        }
        const StructField *field = find_field(base_type->def, expr->as.field.field);
        if (!field) {
            if (out_error && !*out_error) *out_error = dup_error_at(expr, "unknown field");
            return -1;
        }
        if (field->offset == 0) return base_addr;
        int off = ir_emit_const(ir, (long)field->offset);
        if (off < 0) return -1;
        return ir_emit_bin(ir, BIN_ADD, base_addr, off);
    }
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
                        if (locals->items[idx].type && locals->items[idx].type->kind == TYPE_ARRAY) {
                            return addr;
                        }
                        if (locals->items[idx].type && (locals->items[idx].type->kind == TYPE_STRUCT || locals->items[idx].type->kind == TYPE_UNION)) {
                            if (out_error && !*out_error) *out_error = dup_error_at(expr, "struct/union value not supported");
                            return -1;
                        }
                        return emit_load_by_type(ir, addr, locals->items[idx].type);
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
                    if (globals->items[gidx].type && (globals->items[gidx].type->kind == TYPE_STRUCT || globals->items[gidx].type->kind == TYPE_UNION)) {
                        if (out_error && !*out_error) *out_error = dup_error_at(expr, "struct/union value not supported");
                        return -1;
                    }
                    return emit_load_by_type(ir, addr, globals->items[gidx].type);
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
                const Type *t = lvalue_type(expr, locals, globals);
                if (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION)) {
                    if (out_error && !*out_error) *out_error = dup_error_at(expr, "struct/union value not supported");
                    return -1;
                }
                return emit_load_by_type(ir, addr, t);
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
            if (expr->as.bin.op == BIN_ADD || expr->as.bin.op == BIN_SUB) {
                Type *lt = expr_type(expr->as.bin.left, locals, globals);
                Type *rt = expr_type(expr->as.bin.right, locals, globals);
                int lhs = lower_expr(expr->as.bin.left, ir, locals, globals, out_error);
                if (lhs < 0) return -1;
                int rhs = lower_expr(expr->as.bin.right, ir, locals, globals, out_error);
                if (rhs < 0) return -1;
                if (type_is_ptr(lt) && !type_is_ptr(rt)) {
                    int elem = type_size(type_base(lt));
                    if (elem > 1) {
                        int scale = ir_emit_const(ir, elem);
                        if (scale < 0) return -1;
                        rhs = ir_emit_bin(ir, BIN_MUL, rhs, scale);
                        if (rhs < 0) return -1;
                    }
                    return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
                }
                if (expr->as.bin.op == BIN_ADD && !type_is_ptr(lt) && type_is_ptr(rt)) {
                    int elem = type_size(type_base(rt));
                    if (elem > 1) {
                        int scale = ir_emit_const(ir, elem);
                        if (scale < 0) return -1;
                        lhs = ir_emit_bin(ir, BIN_MUL, lhs, scale);
                        if (lhs < 0) return -1;
                    }
                    return ir_emit_bin(ir, BIN_ADD, lhs, rhs);
                }
                if (expr->as.bin.op == BIN_SUB && type_is_ptr(lt) && type_is_ptr(rt)) {
                    int diff = ir_emit_bin(ir, BIN_SUB, lhs, rhs);
                    if (diff < 0) return -1;
                    int elem = type_size(type_base(lt));
                    if (elem > 1) {
                        int scale = ir_emit_const(ir, elem);
                        if (scale < 0) return -1;
                        return ir_emit_bin(ir, BIN_DIV, diff, scale);
                    }
                    return diff;
                }
                return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
            }
            int lhs = lower_expr(expr->as.bin.left, ir, locals, globals, out_error);
            if (lhs < 0) return -1;
            int rhs = lower_expr(expr->as.bin.right, ir, locals, globals, out_error);
            if (rhs < 0) return -1;
            return ir_emit_bin(ir, expr->as.bin.op, lhs, rhs);
        }
        case EXPR_CALL: {
            int prefer = g_prefer_libc;
            const char *fname = expr->as.call.name;
            if (expr->as.call.arg_count == 1 && (!prefer && strcmp(fname, "puts") == 0)) {
                const Expr *arg = expr->as.call.args[0];
                int addr_temp = -1;
                size_t len = 0;
                if (arg->kind == EXPR_STRING) {
                    char *label = dup_label(".Lstr", globals ? globals->str_count++ : 0);
                    if (!label) { if (out_error && !*out_error) *out_error = dup_error_at(expr, "out of memory"); return -1; }
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
            if (expr->as.call.arg_count == 1 &&
                (strcmp(fname, "__minac_putchar") == 0 || (!prefer && strcmp(fname, "putchar") == 0))) {
                int ch_temp = lower_expr(expr->as.call.args[0], ir, locals, globals, out_error);
                if (ch_temp < 0) return -1;
                char *label = dup_label(".Lputc", globals ? globals->byte_count++ : 0);
                if (!label) { if (out_error && !*out_error) *out_error = dup_error_at(expr, "out of memory"); return -1; }
                ir_emit_global_char(ir, label, 0);
                int addr_temp = ir_emit_addr(ir, label);
                if (addr_temp < 0) return -1;
                ir_emit_store8(ir, addr_temp, ch_temp);
                return ir_emit_write(ir, addr_temp, 1);
            }
            if (expr->as.call.arg_count == 1 &&
                (strcmp(fname, "__minac_exit") == 0 || (!prefer && strcmp(fname, "exit") == 0))) {
                int code_temp = lower_expr(expr->as.call.args[0], ir, locals, globals, out_error);
                if (code_temp < 0) return -1;
                ir_emit_exit(ir, code_temp);
                return ir_emit_const(ir, 0);
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
            char *label = dup_label(".Lstr", globals ? globals->str_count++ : 0);
            if (!label) { if (out_error && !*out_error) *out_error = dup_error_at(expr, "out of memory"); return -1; }
            ir_emit_global_str(ir, label, expr->as.str.data, expr->as.str.len);
            return ir_emit_addr(ir, label);
        }
        case EXPR_INDEX: {
            int addr = lower_index_addr(expr, ir, locals, globals, out_error);
            if (addr < 0) return -1;
            const Type *t = lvalue_type(expr, locals, globals);
            if (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION)) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "struct/union value not supported");
                return -1;
            }
            return emit_load_by_type(ir, addr, t);
        }
        case EXPR_FIELD: {
            int addr = lower_addr(expr, ir, locals, globals, out_error);
            if (addr < 0) return -1;
            const Type *t = lvalue_type(expr, locals, globals);
            if (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION)) {
                if (out_error && !*out_error) *out_error = dup_error_at(expr, "struct/union value not supported");
                return -1;
            }
            return emit_load_by_type(ir, addr, t);
        }
        case EXPR_SIZEOF: {
            size_t size = 0;
            if (expr->as.sizeof_expr.type) {
                size = (size_t)type_size(expr->as.sizeof_expr.type);
            } else if (expr->as.sizeof_expr.expr) {
                Type *t = expr_type(expr->as.sizeof_expr.expr, locals, globals);
                size = (size_t)type_size(t);
            }
            return ir_emit_const(ir, (long)size);
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

static int emit_store_offset(IRProgram *ir, int base_addr, size_t offset, int value_temp, const Type *type) {
    int addr = base_addr;
    if (offset != 0) {
        int off = ir_emit_const(ir, (long)offset);
        if (off < 0) return 0;
        addr = ir_emit_bin(ir, BIN_ADD, base_addr, off);
        if (addr < 0) return 0;
    }
    emit_store_by_type(ir, addr, value_temp, type);
    return 1;
}

static int lower_init_list(const Stmt *stmt, IRProgram *ir, LocalMap *locals, GlobalMap *globals, char **out_error) {
    if (!stmt || !stmt->decl_type || !stmt->init_list) return 1;
    Type *type = stmt->decl_type;
    int base_addr = ir_emit_addr(ir, stmt->name);
    if (base_addr < 0) return 0;
    if (type->kind == TYPE_ARRAY) {
        if (!type->base || type->base->kind == TYPE_ARRAY || type->base->kind == TYPE_STRUCT || type->base->kind == TYPE_UNION) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: array initializer element type not supported");
            return 0;
        }
        size_t elem_size = (size_t)type_size(type->base);
        size_t max = type->array_len;
        if (stmt->init_count > max) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: too many array initializers");
            return 0;
        }
        for (size_t i = 0; i < max; i++) {
            int val = -1;
            if (i < stmt->init_count) {
                val = lower_expr(stmt->init_list[i], ir, locals, globals, out_error);
                if (val < 0) return 0;
            } else {
                val = ir_emit_const(ir, 0);
                if (val < 0) return 0;
            }
            if (!emit_store_offset(ir, base_addr, i * elem_size, val, type->base)) return 0;
        }
        return 1;
    }
    if (type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) {
        if (!type->def || type->def->field_count == 0) return 1;
        if (type->kind == TYPE_UNION && stmt->init_count > 1) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: union initializer has too many values");
            return 0;
        }
        if (type->kind == TYPE_STRUCT && stmt->init_count > type->def->field_count) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: too many struct initializer values");
            return 0;
        }
        size_t limit = (type->kind == TYPE_UNION) ? 1 : type->def->field_count;
        for (size_t i = 0; i < limit; i++) {
            const StructField *field = &type->def->fields[i];
            if (field->type->kind == TYPE_STRUCT || field->type->kind == TYPE_UNION || field->type->kind == TYPE_ARRAY) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: nested aggregate initializer not supported");
                return 0;
            }
            int val = -1;
            if (i < stmt->init_count) {
                val = lower_expr(stmt->init_list[i], ir, locals, globals, out_error);
                if (val < 0) return 0;
            } else {
                val = ir_emit_const(ir, 0);
                if (val < 0) return 0;
            }
            if (!emit_store_offset(ir, base_addr, field->offset, val, field->type)) return 0;
            if (type->kind == TYPE_UNION) break;
        }
        return 1;
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
            if (stmt->decl_type && stmt->decl_type->kind == TYPE_ARRAY) {
                size_t size = (size_t)type_size(stmt->decl_type);
                ir_emit_local_alloc(ir, stmt->name, size);
                if (!locals_add(locals, stmt->name, -1, stmt->decl_type, size)) {
                    if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
                    return 0;
                }
                locals->items[locals_find(locals, stmt->name)].addressable = 1;
                if (stmt->init_list) {
                    if (!lower_init_list(stmt, ir, locals, globals, out_error)) return 0;
                }
                return 1;
            }
            if (stmt->decl_type && (stmt->decl_type->kind == TYPE_STRUCT || stmt->decl_type->kind == TYPE_UNION)) {
                size_t size = (size_t)type_size(stmt->decl_type);
                ir_emit_local_alloc(ir, stmt->name, size);
                if (!locals_add(locals, stmt->name, -1, stmt->decl_type, size)) {
                    if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
                    return 0;
                }
                locals->items[locals_find(locals, stmt->name)].addressable = 1;
                if (stmt->init_list) {
                    if (!lower_init_list(stmt, ir, locals, globals, out_error)) return 0;
                }
                return 1;
            }
            if (stmt->init_list) {
                if (out_error && !*out_error) *out_error = dup_error_simple("error: initializer list requires aggregate type");
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
            if (!locals_add(locals, stmt->name, var_temp, stmt->decl_type, (size_t)type_size(stmt->decl_type))) {
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
                const Type *lt = lvalue_type(stmt->lhs, locals, globals);
                emit_store_by_type(ir, addr, expr_temp, lt);
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
                        emit_store_by_type(ir, addr, expr_temp, globals->items[gidx].type);
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
            int temp = -1;
            if (stmt->expr) {
                temp = lower_expr(stmt->expr, ir, locals, globals, out_error);
                if (temp < 0) return 0;
            } else {
                temp = ir_emit_const(ir, 0);
                if (temp < 0) return 0;
            }
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

int lower_program(const Program *program, IRProgram *ir, int prefer_libc, char **out_error) {
    if (out_error) *out_error = NULL;
    g_prefer_libc = prefer_libc;
    if (!program || !program->funcs || program->func_count == 0) {
        if (out_error) *out_error = dup_error_simple("error: invalid program");
        return 0;
    }
    GlobalMap globals = {0};
    for (size_t i = 0; i < program->global_count; i++) {
        Global *g = program->globals[i];
        if (!g) continue;
        if (g->kind == GLOB_INT) ir_emit_global_int(ir, g->name, g->value);
        if (g->kind == GLOB_CHAR) ir_emit_global_char(ir, g->name, (unsigned char)g->value);
        if (g->kind == GLOB_INT_ARR) {
            const long *vals = (const long *)g->data;
            ir_emit_global_int_arr(ir, g->name, vals, (size_t)g->value, g->len);
        }
        if (g->kind == GLOB_STR) ir_emit_global_str(ir, g->name, g->data, g->len);
        if (g->kind == GLOB_BYTES) ir_emit_global_bytes(ir, g->name, g->data, g->len);
        if (!globals_add(&globals, g->name, g->kind, g->len, g->type)) {
            if (out_error && !*out_error) *out_error = dup_error_simple("error: out of memory");
            globals_free(&globals);
            return 0;
        }
    }
    for (size_t f = 0; f < program->func_count; f++) {
        Function *fn = program->funcs[f];
        ir->temp_count = 0;
        ir_emit_func(ir, fn->name);
        LocalMap locals = {0};
        for (size_t i = 0; i < fn->param_count; i++) {
            int temp = ir_new_temp(ir);
            ir_emit_param(ir, (int)i, temp);
            if (!locals_add(&locals, fn->params[i], temp, fn->param_types ? fn->param_types[i] : NULL, (size_t)type_size(fn->param_types ? fn->param_types[i] : NULL))) {
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
