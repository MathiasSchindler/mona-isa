#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const Token *tokens;
    size_t count;
    size_t pos;
    char *error;
    StructDef **structs;
    size_t struct_count;
    size_t struct_cap;
} Parser;

static char *dup_error_at(const Token *tok, const char *msg) {
    char buf[256];
    snprintf(buf, sizeof(buf), "error: %d:%d: %s", tok->line, tok->col, msg);
    size_t len = strlen(buf) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, buf, len);
    return out;
}

static const Token *peek(Parser *p) {
    if (p->pos >= p->count) return &p->tokens[p->count - 1];
    return &p->tokens[p->pos];
}

static const Token *peek_n(Parser *p, size_t n) {
    size_t idx = p->pos + n;
    if (idx >= p->count) return &p->tokens[p->count - 1];
    return &p->tokens[idx];
}

static const Token *advance(Parser *p) {
    if (p->pos < p->count) p->pos++;
    return &p->tokens[p->pos - 1];
}

static int match(Parser *p, TokenKind kind) {
    if (peek(p)->kind == kind) { advance(p); return 1; }
    return 0;
}

static int expect(Parser *p, TokenKind kind, const char *msg) {
    if (match(p, kind)) return 1;
    if (!p->error) p->error = dup_error_at(peek(p), msg);
    return 0;
}

static char *dup_lexeme(const Token *tok) {
    size_t len = (size_t)tok->length;
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, tok->lexeme, len);
    out[len] = '\0';
    return out;
}

static char *dup_string(const char *s) {
    size_t len = strlen(s) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}

static StructDef *find_struct_def(Parser *p, const char *name, int is_union) {
    if (!p || !name) return NULL;
    for (size_t i = 0; i < p->struct_count; i++) {
        StructDef *def = p->structs[i];
        if (!def) continue;
        if (def->is_union != is_union) continue;
        if (strcmp(def->name, name) == 0) return def;
    }
    return NULL;
}

static int add_struct_def(Parser *p, StructDef *def) {
    if (!p || !def) return 0;
    if (p->struct_count + 1 > p->struct_cap) {
        size_t next = (p->struct_cap == 0) ? 8 : p->struct_cap * 2;
        StructDef **mem = (StructDef **)realloc(p->structs, next * sizeof(StructDef *));
        if (!mem) return 0;
        p->structs = mem;
        p->struct_cap = next;
    }
    p->structs[p->struct_count++] = def;
    return 1;
}

static size_t type_align(const Type *type) {
    if (!type) return 8;
    if (type->kind == TYPE_CHAR) return 1;
    if (type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) {
        return type->def ? (type->def->align ? type->def->align : 1) : 1;
    }
    if (type->kind == TYPE_ARRAY) return type_align(type->base);
    return 8;
}

static size_t align_up_size(size_t v, size_t a) {
    if (a == 0) return v;
    size_t m = a - 1;
    return (v + m) & ~m;
}

static size_t type_size_local(const Type *type) {
    if (!type) return 8;
    switch (type->kind) {
        case TYPE_CHAR:
            return 1;
        case TYPE_VOID:
            return 0;
        case TYPE_ARRAY:
            return type_size_local(type->base) * type->array_len;
        case TYPE_STRUCT:
        case TYPE_UNION:
            return type->def ? type->def->size : 0;
        default:
            return 8;
    }
}

static Expr *new_expr(ExprKind kind, int line, int col) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (!expr) return NULL;
    expr->kind = kind;
    expr->line = line;
    expr->col = col;
    return expr;
}

static Type *new_type(TypeKind kind, Type *base) {
    Type *type = (Type *)calloc(1, sizeof(Type));
    if (!type) return NULL;
    type->kind = kind;
    type->base = base;
    return type;
}

static void free_type_local(Type *type) {
    if (!type) return;
    if (type->base) free_type_local(type->base);
    free(type);
}

static void free_expr_local(Expr *expr) {
    if (!expr) return;
    switch (expr->kind) {
        case EXPR_IDENT:
            free(expr->as.ident);
            break;
        case EXPR_BINARY:
            free_expr_local(expr->as.bin.left);
            free_expr_local(expr->as.bin.right);
            break;
        case EXPR_UNARY:
            free_expr_local(expr->as.unary.expr);
            break;
        case EXPR_CALL:
            free(expr->as.call.name);
            for (size_t i = 0; i < expr->as.call.arg_count; i++) free_expr_local(expr->as.call.args[i]);
            free(expr->as.call.args);
            break;
        case EXPR_STRING:
            free(expr->as.str.data);
            break;
        case EXPR_INDEX:
            free_expr_local(expr->as.index.base);
            free_expr_local(expr->as.index.index);
            break;
        case EXPR_FIELD:
            free_expr_local(expr->as.field.base);
            free(expr->as.field.field);
            break;
        case EXPR_SIZEOF:
            if (expr->as.sizeof_expr.type) free_type_local(expr->as.sizeof_expr.type);
            if (expr->as.sizeof_expr.expr) free_expr_local(expr->as.sizeof_expr.expr);
            break;
        case EXPR_NUMBER:
        default:
            break;
    }
    free(expr);
}

static Stmt *new_stmt(StmtKind kind, char *name, Expr *expr) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->kind = kind;
    stmt->name = name;
    stmt->expr = expr;
    return stmt;
}

static Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->kind = STMT_IF;
    stmt->cond = cond;
    stmt->then_branch = then_branch;
    stmt->else_branch = else_branch;
    return stmt;
}

static Stmt *new_stmt_while(Expr *cond, Stmt *body) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->kind = STMT_WHILE;
    stmt->cond = cond;
    stmt->body = body;
    return stmt;
}

static Stmt *new_stmt_block(Stmt **stmts, size_t count) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->kind = STMT_BLOCK;
    stmt->stmts = stmts;
    stmt->stmt_count = count;
    return stmt;
}

static void free_stmt_local(Stmt *stmt) {
    if (!stmt) return;
    if (stmt->kind == STMT_DECL || stmt->kind == STMT_ASSIGN) free(stmt->name);
    if (stmt->expr) free_expr_local(stmt->expr);
    if (stmt->lhs) free_expr_local(stmt->lhs);
    if (stmt->index) free_expr_local(stmt->index);
    if (stmt->cond) free_expr_local(stmt->cond);
    if (stmt->then_branch) free_stmt_local(stmt->then_branch);
    if (stmt->else_branch) free_stmt_local(stmt->else_branch);
    if (stmt->body) free_stmt_local(stmt->body);
    if (stmt->init) free_stmt_local(stmt->init);
    if (stmt->post) free_stmt_local(stmt->post);
    if (stmt->decl_type) free_type_local(stmt->decl_type);
    if (stmt->stmts) {
        for (size_t i = 0; i < stmt->stmt_count; i++) free_stmt_local(stmt->stmts[i]);
        free(stmt->stmts);
    }
    if (stmt->cases) {
        for (size_t i = 0; i < stmt->case_count; i++) {
            for (size_t j = 0; j < stmt->cases[i].stmt_count; j++) free_stmt_local(stmt->cases[i].stmts[j]);
            free(stmt->cases[i].stmts);
        }
        free(stmt->cases);
    }
    if (stmt->default_stmts) {
        for (size_t i = 0; i < stmt->default_count; i++) free_stmt_local(stmt->default_stmts[i]);
        free(stmt->default_stmts);
    }
    free(stmt);
}

static void free_function_local(Function *func) {
    if (!func) return;
    free(func->name);
    for (size_t i = 0; i < func->param_count; i++) free(func->params[i]);
    free(func->params);
    if (func->param_types) {
        for (size_t i = 0; i < func->param_count; i++) free_type_local(func->param_types[i]);
        free(func->param_types);
    }
    if (func->ret_type) free_type_local(func->ret_type);
    for (size_t i = 0; i < func->stmt_count; i++) free_stmt_local(func->stmts[i]);
    free(func->stmts);
    free(func);
}

static Function *new_function(char *name) {
    Function *fn = (Function *)calloc(1, sizeof(Function));
    if (!fn) return NULL;
    fn->name = name;
    return fn;
}

static Expr *parse_expr(Parser *p);

static Type *parse_type(Parser *p) {
    Type *base = NULL;
    if (match(p, TOK_INT)) base = new_type(TYPE_INT, NULL);
    else if (match(p, TOK_CHAR)) base = new_type(TYPE_CHAR, NULL);
    else if (match(p, TOK_VOID)) base = new_type(TYPE_VOID, NULL);
    else if (match(p, TOK_STRUCT) || match(p, TOK_UNION)) {
        const Token *kw = p->tokens + (p->pos - 1);
        int is_union = (kw->kind == TOK_UNION);
        if (peek(p)->kind != TOK_IDENT) {
            if (!p->error) p->error = dup_error_at(peek(p), "expected struct/union name");
            return NULL;
        }
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
        advance(p);

        StructDef *def = find_struct_def(p, name, is_union);
        if (match(p, TOK_LBRACE)) {
            if (def && def->field_count > 0) {
                free(name);
                if (!p->error) p->error = dup_error_at(peek(p), "struct/union redefinition");
                return NULL;
            }
            if (!def) {
                def = (StructDef *)calloc(1, sizeof(StructDef));
                if (!def) { free(name); p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
                def->name = name;
                def->is_union = is_union;
                if (!add_struct_def(p, def)) {
                    free(def->name);
                    free(def);
                    p->error = dup_error_at(peek(p), "out of memory");
                    return NULL;
                }
            } else {
                free(name);
            }

            StructField *fields = NULL;
            size_t field_count = 0;
            size_t field_cap = 0;
            size_t offset = 0;
            size_t max_align = 1;
            size_t max_size = 0;
            while (peek(p)->kind != TOK_RBRACE && peek(p)->kind != TOK_EOF) {
                Type *ftype = parse_type(p);
                if (!ftype) { p->error = dup_error_at(peek(p), "expected field type"); goto struct_fail; }
                if (ftype->kind == TYPE_VOID) { free_type_local(ftype); p->error = dup_error_at(peek(p), "void field not supported"); goto struct_fail; }
                if (peek(p)->kind != TOK_IDENT) { free_type_local(ftype); p->error = dup_error_at(peek(p), "expected field name"); goto struct_fail; }
                char *fname = dup_lexeme(peek(p));
                if (!fname) { free_type_local(ftype); p->error = dup_error_at(peek(p), "out of memory"); goto struct_fail; }
                advance(p);
                if (match(p, TOK_LBRACKET)) {
                    if (peek(p)->kind != TOK_NUMBER) { free(fname); free_type_local(ftype); p->error = dup_error_at(peek(p), "expected array size"); goto struct_fail; }
                    long count = peek(p)->value;
                    advance(p);
                    if (!expect(p, TOK_RBRACKET, "expected ']'")) { free(fname); free_type_local(ftype); goto struct_fail; }
                    Type *arr = new_type(TYPE_ARRAY, ftype);
                    if (!arr) { free(fname); free_type_local(ftype); p->error = dup_error_at(peek(p), "out of memory"); goto struct_fail; }
                    arr->array_len = (size_t)count;
                    ftype = arr;
                }
                if (!expect(p, TOK_SEMI, "expected ';' after field")) { free(fname); free_type_local(ftype); goto struct_fail; }

                if (field_count + 1 > field_cap) {
                    size_t next = (field_cap == 0) ? 4 : field_cap * 2;
                    StructField *mem = (StructField *)realloc(fields, next * sizeof(StructField));
                    if (!mem) { free(fname); free_type_local(ftype); p->error = dup_error_at(peek(p), "out of memory"); goto struct_fail; }
                    fields = mem;
                    field_cap = next;
                }
                size_t fsize = type_size_local(ftype);
                size_t falign = type_align(ftype);
                if (def->is_union) {
                    fields[field_count].offset = 0;
                    if (fsize > max_size) max_size = fsize;
                } else {
                    offset = align_up_size(offset, falign);
                    fields[field_count].offset = offset;
                    offset += fsize;
                }
                if (falign > max_align) max_align = falign;
                fields[field_count].name = fname;
                fields[field_count].type = ftype;
                fields[field_count].size = fsize;
                field_count++;
                continue;

struct_fail:
                for (size_t i = 0; i < field_count; i++) {
                    free(fields[i].name);
                    if (fields[i].type) free_type_local(fields[i].type);
                }
                free(fields);
                return NULL;
            }
            if (!expect(p, TOK_RBRACE, "expected '}'")) {
                for (size_t i = 0; i < field_count; i++) {
                    free(fields[i].name);
                    if (fields[i].type) free_type_local(fields[i].type);
                }
                free(fields);
                return NULL;
            }
            def->fields = fields;
            def->field_count = field_count;
            def->align = max_align;
            if (def->is_union) {
                def->size = align_up_size(max_size, max_align);
            } else {
                def->size = align_up_size(offset, max_align);
            }
        } else {
            if (!def) {
                free(name);
                if (!p->error) p->error = dup_error_at(peek(p), "unknown struct/union");
                return NULL;
            }
            free(name);
        }

        base = new_type(is_union ? TYPE_UNION : TYPE_STRUCT, NULL);
        if (!base) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
        base->def = def;
    }
    if (!base) return NULL;
    while (match(p, TOK_STAR)) {
        Type *ptr = new_type(TYPE_PTR, base);
        if (!ptr) { free_type_local(base); return NULL; }
        base = ptr;
    }
    return base;
}

static int decode_string(const Token *tok, char **out_data, size_t *out_len, char **out_error) {
    if (tok->kind != TOK_STRING || tok->length < 2) return 0;
    const char *s = tok->lexeme + 1;
    size_t len = (size_t)tok->length - 2;
    char *buf = (char *)malloc(len + 1);
    if (!buf) {
        if (out_error && !*out_error) *out_error = dup_error_at(tok, "out of memory");
        return 0;
    }
    size_t out = 0;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\\' && i + 1 < len) {
            char n = s[++i];
            if (n == 'n') buf[out++] = '\n';
            else if (n == 't') buf[out++] = '\t';
            else if (n == '\\') buf[out++] = '\\';
            else if (n == '"') buf[out++] = '"';
            else buf[out++] = n;
        } else {
            buf[out++] = c;
        }
    }
    buf[out] = '\0';
    *out_data = buf;
    *out_len = out;
    return 1;
}

static Expr *parse_primary(Parser *p) {
    const Token *tok = peek(p);
    if (match(p, TOK_NUMBER)) {
        Expr *expr = new_expr(EXPR_NUMBER, tok->line, tok->col);
        if (!expr) return NULL;
        expr->as.number = tok->value;
        return expr;
    }
    if (match(p, TOK_IDENT)) {
        Expr *expr = new_expr(EXPR_IDENT, tok->line, tok->col);
        if (!expr) return NULL;
        expr->as.ident = dup_lexeme(tok);
        if (!expr->as.ident) { free(expr); return NULL; }
        return expr;
    }
    if (match(p, TOK_STRING)) {
        Expr *expr = new_expr(EXPR_STRING, tok->line, tok->col);
        if (!expr) return NULL;
        char *data = NULL;
        size_t len = 0;
        if (!decode_string(tok, &data, &len, &p->error)) { free(expr); return NULL; }
        expr->as.str.data = data;
        expr->as.str.len = len;
        return expr;
    }
    if (match(p, TOK_LPAREN)) {
        Expr *inner = parse_expr(p);
        if (!inner) return NULL;
        if (!expect(p, TOK_RPAREN, "expected ')'") ) { free_expr_local(inner); return NULL; }
        return inner;
    }
    if (!p->error) p->error = dup_error_at(peek(p), "expected primary expression");
    return NULL;
}

static Expr *parse_postfix(Parser *p) {
    Expr *expr = parse_primary(p);
    if (!expr) return NULL;
    while (1) {
        if (match(p, TOK_LPAREN)) {
            if (expr->kind != EXPR_IDENT) {
                free_expr_local(expr);
                if (!p->error) p->error = dup_error_at(peek(p), "call target must be identifier");
                return NULL;
            }
            Expr **args = NULL;
            size_t count = 0;
            size_t cap = 0;
            if (peek(p)->kind != TOK_RPAREN) {
                while (1) {
                    Expr *arg = parse_expr(p);
                    if (!arg) {
                        for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                        free(args);
                        free_expr_local(expr);
                        return NULL;
                    }
                    if (count + 1 > cap) {
                        size_t next = (cap == 0) ? 4 : cap * 2;
                        Expr **mem = (Expr **)realloc(args, next * sizeof(Expr *));
                        if (!mem) {
                            free_expr_local(arg);
                            for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                            free(args);
                            free_expr_local(expr);
                            p->error = dup_error_at(peek(p), "out of memory");
                            return NULL;
                        }
                        args = mem;
                        cap = next;
                    }
                    args[count++] = arg;
                    if (match(p, TOK_COMMA)) continue;
                    break;
                }
            }
            if (!expect(p, TOK_RPAREN, "expected ')'") ) {
                for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                free(args);
                free_expr_local(expr);
                return NULL;
            }
            Expr *call = new_expr(EXPR_CALL, expr->line, expr->col);
            if (!call) {
                for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                free(args);
                free_expr_local(expr);
                return NULL;
            }
            call->as.call.name = dup_string(expr->as.ident);
            free_expr_local(expr);
            if (!call->as.call.name) { free_expr_local(call); return NULL; }
            call->as.call.args = args;
            call->as.call.arg_count = count;
            expr = call;
            continue;
        }
        if (match(p, TOK_LBRACKET)) {
            Expr *idx = parse_expr(p);
            if (!idx) { free_expr_local(expr); return NULL; }
            if (!expect(p, TOK_RBRACKET, "expected ']'")) { free_expr_local(expr); free_expr_local(idx); return NULL; }
            Expr *ind = new_expr(EXPR_INDEX, expr->line, expr->col);
            if (!ind) { free_expr_local(expr); free_expr_local(idx); return NULL; }
            ind->as.index.base = expr;
            ind->as.index.index = idx;
            expr = ind;
            continue;
        }
        if (match(p, TOK_DOT) || match(p, TOK_ARROW)) {
            const Token *op = p->tokens + (p->pos - 1);
            if (peek(p)->kind != TOK_IDENT) {
                free_expr_local(expr);
                if (!p->error) p->error = dup_error_at(peek(p), "expected field name");
                return NULL;
            }
            char *fname = dup_lexeme(peek(p));
            if (!fname) { free_expr_local(expr); p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
            advance(p);
            Expr *field = new_expr(EXPR_FIELD, op->line, op->col);
            if (!field) { free_expr_local(expr); free(fname); return NULL; }
            field->as.field.base = expr;
            field->as.field.field = fname;
            field->as.field.is_arrow = (op->kind == TOK_ARROW);
            expr = field;
            continue;
        }
        break;
    }
    return expr;
}

static Expr *parse_unary(Parser *p) {
    const Token *tok = peek(p);
    if (match(p, TOK_SIZEOF)) {
        Expr *expr = new_expr(EXPR_SIZEOF, tok->line, tok->col);
        if (!expr) return NULL;
        expr->as.sizeof_expr.type = NULL;
        expr->as.sizeof_expr.expr = NULL;
        if (match(p, TOK_LPAREN)) {
            if (peek(p)->kind == TOK_INT || peek(p)->kind == TOK_CHAR || peek(p)->kind == TOK_VOID || peek(p)->kind == TOK_STRUCT || peek(p)->kind == TOK_UNION) {
                Type *type = parse_type(p);
                if (!type) { free_expr_local(expr); return NULL; }
                if (!expect(p, TOK_RPAREN, "expected ')'")) { free_type_local(type); free_expr_local(expr); return NULL; }
                expr->as.sizeof_expr.type = type;
                return expr;
            }
            Expr *inner = parse_expr(p);
            if (!inner) { free_expr_local(expr); return NULL; }
            if (!expect(p, TOK_RPAREN, "expected ')'")) { free_expr_local(inner); free_expr_local(expr); return NULL; }
            expr->as.sizeof_expr.expr = inner;
            return expr;
        }
        Expr *inner = parse_unary(p);
        if (!inner) { free_expr_local(expr); return NULL; }
        expr->as.sizeof_expr.expr = inner;
        return expr;
    }
    if (match(p, TOK_PLUS) || match(p, TOK_MINUS) || match(p, TOK_NOT) || match(p, TOK_AMP) || match(p, TOK_STAR)) {
        Expr *rhs = parse_unary(p);
        if (!rhs) return NULL;
        Expr *expr = new_expr(EXPR_UNARY, tok->line, tok->col);
        if (!expr) { free_expr_local(rhs); return NULL; }
        if (tok->kind == TOK_MINUS) expr->as.unary.op = UN_MINUS;
        else if (tok->kind == TOK_NOT) expr->as.unary.op = UN_NOT;
        else if (tok->kind == TOK_AMP) expr->as.unary.op = UN_ADDR;
        else if (tok->kind == TOK_STAR) expr->as.unary.op = UN_DEREF;
        else expr->as.unary.op = UN_PLUS;
        expr->as.unary.expr = rhs;
        return expr;
    }
    return parse_postfix(p);
}

static Expr *parse_term(Parser *p) {
    Expr *expr = parse_unary(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_STAR || peek(p)->kind == TOK_SLASH) {
        const Token *op = advance(p);
        Expr *rhs = parse_unary(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        bin->as.bin.op = (op->kind == TOK_STAR) ? BIN_MUL : BIN_DIV;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_add(Parser *p) {
    Expr *expr = parse_term(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_PLUS || peek(p)->kind == TOK_MINUS) {
        const Token *op = advance(p);
        Expr *rhs = parse_term(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        bin->as.bin.op = (op->kind == TOK_PLUS) ? BIN_ADD : BIN_SUB;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_relational(Parser *p) {
    Expr *expr = parse_add(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_LT || peek(p)->kind == TOK_LTE || peek(p)->kind == TOK_GT || peek(p)->kind == TOK_GTE) {
        const Token *op = advance(p);
        Expr *rhs = parse_add(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        if (op->kind == TOK_LT) bin->as.bin.op = BIN_LT;
        else if (op->kind == TOK_LTE) bin->as.bin.op = BIN_LTE;
        else if (op->kind == TOK_GT) bin->as.bin.op = BIN_GT;
        else bin->as.bin.op = BIN_GTE;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_equality(Parser *p) {
    Expr *expr = parse_relational(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_EQ || peek(p)->kind == TOK_NEQ) {
        const Token *op = advance(p);
        Expr *rhs = parse_relational(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        bin->as.bin.op = (op->kind == TOK_EQ) ? BIN_EQ : BIN_NEQ;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_logand(Parser *p) {
    Expr *expr = parse_equality(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_ANDAND) {
        const Token *op = advance(p);
        Expr *rhs = parse_equality(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        bin->as.bin.op = BIN_LOGAND;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_logor(Parser *p) {
    Expr *expr = parse_logand(p);
    if (!expr) return NULL;
    while (peek(p)->kind == TOK_OROR) {
        const Token *op = advance(p);
        Expr *rhs = parse_logand(p);
        if (!rhs) { free_expr_local(expr); return NULL; }
        Expr *bin = new_expr(EXPR_BINARY, op->line, op->col);
        if (!bin) { free_expr_local(expr); free_expr_local(rhs); return NULL; }
        bin->as.bin.op = BIN_LOGOR;
        bin->as.bin.left = expr;
        bin->as.bin.right = rhs;
        expr = bin;
    }
    return expr;
}

static Expr *parse_expr(Parser *p) {
    return parse_logor(p);
}

static Expr *parse_cond(Parser *p) {
    return parse_expr(p);
}

static int parse_stmt_list(Parser *p, Stmt ***out_stmts, size_t *out_count);

static int is_lvalue_expr(const Expr *expr) {
    if (!expr) return 0;
    if (expr->kind == EXPR_IDENT || expr->kind == EXPR_INDEX || expr->kind == EXPR_FIELD) return 1;
    if (expr->kind == EXPR_UNARY && expr->as.unary.op == UN_DEREF) return 1;
    return 0;
}

static Expr *parse_lvalue(Parser *p) {
    Expr *expr = parse_unary(p);
    if (!expr) return NULL;
    if (!is_lvalue_expr(expr)) {
        if (!p->error) p->error = dup_error_at(peek(p), "expected lvalue");
        free_expr_local(expr);
        return NULL;
    }
    return expr;
}

static Stmt *parse_stmt(Parser *p) {
    if (match(p, TOK_LBRACE)) {
        Stmt **stmts = NULL;
        size_t count = 0;
        if (!parse_stmt_list(p, &stmts, &count)) return NULL;
        if (!expect(p, TOK_RBRACE, "expected '}'")) {
            for (size_t i = 0; i < count; i++) free_stmt_local(stmts[i]);
            free(stmts);
            return NULL;
        }
        return new_stmt_block(stmts, count);
    }
    if (match(p, TOK_IF)) {
        if (!expect(p, TOK_LPAREN, "expected '('") ) return NULL;
        Expr *cond = parse_cond(p);
        if (!cond) return NULL;
        if (!expect(p, TOK_RPAREN, "expected ')'") ) { free_expr_local(cond); return NULL; }
        Stmt *then_branch = parse_stmt(p);
        if (!then_branch) { free_expr_local(cond); return NULL; }
        Stmt *else_branch = NULL;
        if (match(p, TOK_ELSE)) {
            else_branch = parse_stmt(p);
            if (!else_branch) { free_expr_local(cond); free_stmt_local(then_branch); return NULL; }
        }
        return new_stmt_if(cond, then_branch, else_branch);
    }
    if (match(p, TOK_WHILE)) {
        if (!expect(p, TOK_LPAREN, "expected '('") ) return NULL;
        Expr *cond = parse_cond(p);
        if (!cond) return NULL;
        if (!expect(p, TOK_RPAREN, "expected ')'") ) { free_expr_local(cond); return NULL; }
        Stmt *body = parse_stmt(p);
        if (!body) { free_expr_local(cond); return NULL; }
        return new_stmt_while(cond, body);
    }
    if (match(p, TOK_FOR)) {
        if (!expect(p, TOK_LPAREN, "expected '('") ) return NULL;
        Stmt *init = NULL;
        if (!match(p, TOK_SEMI)) {
            if (peek(p)->kind == TOK_INT || peek(p)->kind == TOK_CHAR || peek(p)->kind == TOK_VOID || peek(p)->kind == TOK_STRUCT || peek(p)->kind == TOK_UNION) {
                Type *type = parse_type(p);
                if (!type) return NULL;
                if (type->kind == TYPE_VOID) { free_type_local(type); if (!p->error) p->error = dup_error_at(peek(p), "void variable not supported"); return NULL; }
                if (peek(p)->kind != TOK_IDENT) { expect(p, TOK_IDENT, "expected identifier"); return NULL; }
                char *name = dup_lexeme(peek(p));
                if (!name) { free_type_local(type); p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
                advance(p);
                Expr *val = NULL;
                if (match(p, TOK_ASSIGN)) {
                    val = parse_expr(p);
                    if (!val) { free(name); free_type_local(type); return NULL; }
                }
                init = new_stmt(STMT_DECL, name, val);
                if (!init) { free(name); free_expr_local(val); free_type_local(type); return NULL; }
                init->decl_type = type;
            } else if (peek(p)->kind == TOK_IDENT && (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET || peek_n(p, 1)->kind == TOK_DOT || peek_n(p, 1)->kind == TOK_ARROW)) {
                Expr *lhs = parse_lvalue(p);
                if (!lhs) return NULL;
                if (!expect(p, TOK_ASSIGN, "expected '='")) { free_expr_local(lhs); return NULL; }
                Expr *val = parse_expr(p);
                if (!val) { free_expr_local(lhs); return NULL; }
                init = new_stmt(STMT_ASSIGN, NULL, val);
                if (!init) { free_expr_local(lhs); free_expr_local(val); return NULL; }
                init->lhs = lhs;
            } else {
                Expr *expr = parse_expr(p);
                if (!expr) return NULL;
                init = new_stmt(STMT_EXPR, NULL, expr);
                if (!init) { free_expr_local(expr); return NULL; }
            }
            if (!expect(p, TOK_SEMI, "expected ';'")) { free_stmt_local(init); return NULL; }
        }

        Expr *cond = NULL;
        if (!match(p, TOK_SEMI)) {
            cond = parse_expr(p);
            if (!cond) { free_stmt_local(init); return NULL; }
            if (!expect(p, TOK_SEMI, "expected ';'")) { free_stmt_local(init); free_expr_local(cond); return NULL; }
        }

        Stmt *post = NULL;
        if (!match(p, TOK_RPAREN)) {
            if (peek(p)->kind == TOK_IDENT && (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET || peek_n(p, 1)->kind == TOK_DOT || peek_n(p, 1)->kind == TOK_ARROW)) {
                Expr *lhs = parse_lvalue(p);
                if (!lhs) { free_stmt_local(init); free_expr_local(cond); return NULL; }
                if (!expect(p, TOK_ASSIGN, "expected '='")) { free_stmt_local(init); free_expr_local(cond); free_expr_local(lhs); return NULL; }
                Expr *val = parse_expr(p);
                if (!val) { free_stmt_local(init); free_expr_local(cond); free_expr_local(lhs); return NULL; }
                post = new_stmt(STMT_ASSIGN, NULL, val);
                if (!post) { free_stmt_local(init); free_expr_local(cond); free_expr_local(lhs); free_expr_local(val); return NULL; }
                post->lhs = lhs;
            } else {
                Expr *expr = parse_expr(p);
                if (!expr) { free_stmt_local(init); free_expr_local(cond); return NULL; }
                post = new_stmt(STMT_EXPR, NULL, expr);
                if (!post) { free_stmt_local(init); free_expr_local(cond); free_expr_local(expr); return NULL; }
            }
            if (!expect(p, TOK_RPAREN, "expected ')'")) { free_stmt_local(init); free_expr_local(cond); free_stmt_local(post); return NULL; }
        }

        Stmt *body = parse_stmt(p);
        if (!body) { free_stmt_local(init); free_expr_local(cond); free_stmt_local(post); return NULL; }
        Stmt *stmt = new_stmt(STMT_FOR, NULL, NULL);
        if (!stmt) { free_stmt_local(init); free_expr_local(cond); free_stmt_local(post); free_stmt_local(body); return NULL; }
        stmt->init = init;
        stmt->cond = cond;
        stmt->post = post;
        stmt->body = body;
        return stmt;
    }
    if (match(p, TOK_SWITCH)) {
        if (!expect(p, TOK_LPAREN, "expected '('") ) return NULL;
        Expr *cond = parse_expr(p);
        if (!cond) return NULL;
        if (!expect(p, TOK_RPAREN, "expected ')'") ) { free_expr_local(cond); return NULL; }
        if (!expect(p, TOK_LBRACE, "expected '{'") ) { free_expr_local(cond); return NULL; }

        SwitchCase *cases = NULL;
        size_t case_count = 0;
        Stmt **def_stmts = NULL;
        size_t def_count = 0;
        while (peek(p)->kind != TOK_RBRACE && peek(p)->kind != TOK_EOF) {
            if (match(p, TOK_CASE)) {
                if (peek(p)->kind != TOK_NUMBER) { expect(p, TOK_NUMBER, "expected case value"); goto switch_fail; }
                long val = peek(p)->value;
                advance(p);
                if (!expect(p, TOK_COLON, "expected ':'")) goto switch_fail;
                Stmt **stmts = NULL;
                size_t stmt_count = 0;
                while (peek(p)->kind != TOK_CASE && peek(p)->kind != TOK_DEFAULT && peek(p)->kind != TOK_RBRACE) {
                    Stmt *s = parse_stmt(p);
                    if (!s) goto switch_fail;
                    Stmt **mem = (Stmt **)realloc(stmts, (stmt_count + 1) * sizeof(Stmt *));
                    if (!mem) { p->error = dup_error_at(peek(p), "out of memory"); goto switch_fail; }
                    stmts = mem;
                    stmts[stmt_count++] = s;
                }
                SwitchCase *cmem = (SwitchCase *)realloc(cases, (case_count + 1) * sizeof(SwitchCase));
                if (!cmem) { p->error = dup_error_at(peek(p), "out of memory"); goto switch_fail; }
                cases = cmem;
                cases[case_count].value = val;
                cases[case_count].stmts = stmts;
                cases[case_count].stmt_count = stmt_count;
                case_count++;
                continue;
            }
            if (match(p, TOK_DEFAULT)) {
                if (!expect(p, TOK_COLON, "expected ':'")) goto switch_fail;
                while (peek(p)->kind != TOK_CASE && peek(p)->kind != TOK_DEFAULT && peek(p)->kind != TOK_RBRACE) {
                    Stmt *s = parse_stmt(p);
                    if (!s) goto switch_fail;
                    Stmt **mem = (Stmt **)realloc(def_stmts, (def_count + 1) * sizeof(Stmt *));
                    if (!mem) { p->error = dup_error_at(peek(p), "out of memory"); goto switch_fail; }
                    def_stmts = mem;
                    def_stmts[def_count++] = s;
                }
                continue;
            }
            p->error = dup_error_at(peek(p), "expected case/default");
            goto switch_fail;
        }
        if (!expect(p, TOK_RBRACE, "expected '}'")) goto switch_fail;
        Stmt *stmt = new_stmt(STMT_SWITCH, NULL, NULL);
        if (!stmt) goto switch_fail;
        stmt->cond = cond;
        stmt->cases = cases;
        stmt->case_count = case_count;
        stmt->default_stmts = def_stmts;
        stmt->default_count = def_count;
        return stmt;

switch_fail:
        for (size_t i = 0; i < case_count; i++) {
            for (size_t j = 0; j < cases[i].stmt_count; j++) free_stmt_local(cases[i].stmts[j]);
            free(cases[i].stmts);
        }
        free(cases);
        for (size_t i = 0; i < def_count; i++) free_stmt_local(def_stmts[i]);
        free(def_stmts);
        free_expr_local(cond);
        return NULL;
    }
    if (match(p, TOK_BREAK)) {
        if (!expect(p, TOK_SEMI, "expected ';' after break")) return NULL;
        return new_stmt(STMT_BREAK, NULL, NULL);
    }
    if (match(p, TOK_CONTINUE)) {
        if (!expect(p, TOK_SEMI, "expected ';' after continue")) return NULL;
        return new_stmt(STMT_CONTINUE, NULL, NULL);
    }
    if (match(p, TOK_RETURN)) {
        Expr *expr = NULL;
        if (!match(p, TOK_SEMI)) {
            expr = parse_expr(p);
            if (!expr) return NULL;
            if (!expect(p, TOK_SEMI, "expected ';' after return")) { free_expr_local(expr); return NULL; }
        }
        return new_stmt(STMT_RETURN, NULL, expr);
    }
    if (peek(p)->kind == TOK_INT || peek(p)->kind == TOK_CHAR || peek(p)->kind == TOK_VOID || peek(p)->kind == TOK_STRUCT || peek(p)->kind == TOK_UNION) {
        Type *type = parse_type(p);
        if (!type) return NULL;
        if (type->kind == TYPE_VOID) { free_type_local(type); if (!p->error) p->error = dup_error_at(peek(p), "void variable not supported"); return NULL; }
        if (peek(p)->kind != TOK_IDENT) {
            expect(p, TOK_IDENT, "expected identifier");
            return NULL;
        }
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
        advance(p);
        if (match(p, TOK_LBRACKET)) {
            if (peek(p)->kind != TOK_NUMBER) { expect(p, TOK_NUMBER, "expected array size"); free(name); free_type_local(type); return NULL; }
            long count = peek(p)->value;
            advance(p);
            if (!expect(p, TOK_RBRACKET, "expected ']'")) { free(name); free_type_local(type); return NULL; }
            Type *arr = new_type(TYPE_ARRAY, type);
            if (!arr) { free(name); free_type_local(type); return NULL; }
            arr->array_len = (size_t)count;
            if (match(p, TOK_ASSIGN)) {
                if (!p->error) p->error = dup_error_at(peek(p), "array initializer not supported for locals");
                free(name);
                free_type_local(arr);
                return NULL;
            }
            if (!expect(p, TOK_SEMI, "expected ';' after declaration")) { free(name); free_type_local(arr); return NULL; }
            Stmt *stmt = new_stmt(STMT_DECL, name, NULL);
            if (!stmt) { free(name); free_type_local(arr); return NULL; }
            stmt->decl_type = arr;
            return stmt;
        }
        Expr *init = NULL;
        if (match(p, TOK_ASSIGN)) {
            init = parse_expr(p);
            if (!init) { free(name); return NULL; }
        }
        if (!expect(p, TOK_SEMI, "expected ';' after declaration")) { free(name); free_expr_local(init); return NULL; }
        Stmt *stmt = new_stmt(STMT_DECL, name, init);
        if (!stmt) { free(name); free_expr_local(init); free_type_local(type); return NULL; }
        stmt->decl_type = type;
        return stmt;
    }
    if (peek(p)->kind == TOK_IDENT) {
        if (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET || peek_n(p, 1)->kind == TOK_DOT || peek_n(p, 1)->kind == TOK_ARROW) {
            Expr *lhs = parse_lvalue(p);
            if (!lhs) return NULL;
            if (!expect(p, TOK_ASSIGN, "expected '='")) { free_expr_local(lhs); return NULL; }
            Expr *value = parse_expr(p);
            if (!value) { free_expr_local(lhs); return NULL; }
            if (!expect(p, TOK_SEMI, "expected ';' after assignment")) { free_expr_local(lhs); free_expr_local(value); return NULL; }
            Stmt *stmt = new_stmt(STMT_ASSIGN, NULL, value);
            if (!stmt) { free_expr_local(lhs); free_expr_local(value); return NULL; }
            stmt->lhs = lhs;
            return stmt;
        }
    }
    if (peek(p)->kind == TOK_STAR) {
        Expr *lhs = parse_lvalue(p);
        if (!lhs) return NULL;
        if (!expect(p, TOK_ASSIGN, "expected '='")) { free_expr_local(lhs); return NULL; }
        Expr *value = parse_expr(p);
        if (!value) { free_expr_local(lhs); return NULL; }
        if (!expect(p, TOK_SEMI, "expected ';' after assignment")) { free_expr_local(lhs); free_expr_local(value); return NULL; }
        Stmt *stmt = new_stmt(STMT_ASSIGN, NULL, value);
        if (!stmt) { free_expr_local(lhs); free_expr_local(value); return NULL; }
        stmt->lhs = lhs;
        return stmt;
    }
    Expr *expr = parse_expr(p);
    if (!expr) return NULL;
    if (!expect(p, TOK_SEMI, "expected ';' after expression")) { free_expr_local(expr); return NULL; }
    return new_stmt(STMT_EXPR, NULL, expr);
}

static int parse_stmt_list(Parser *p, Stmt ***out_stmts, size_t *out_count) {
    size_t cap = 0;
    size_t count = 0;
    Stmt **stmts = NULL;
    while (peek(p)->kind != TOK_RBRACE && peek(p)->kind != TOK_EOF) {
        Stmt *stmt = parse_stmt(p);
        if (!stmt) {
            for (size_t i = 0; i < count; i++) free_stmt_local(stmts[i]);
            free(stmts);
            return 0;
        }
        if (count + 1 > cap) {
            size_t next = (cap == 0) ? 8 : cap * 2;
            Stmt **mem = (Stmt **)realloc(stmts, next * sizeof(Stmt *));
            if (!mem) {
                free_stmt_local(stmt);
                for (size_t i = 0; i < count; i++) free_stmt_local(stmts[i]);
                free(stmts);
                p->error = dup_error_at(peek(p), "out of memory");
                return 0;
            }
            stmts = mem;
            cap = next;
        }
        stmts[count++] = stmt;
    }
    *out_stmts = stmts;
    *out_count = count;
    return 1;
}

static int parse_params(Parser *p, char ***out_params, Type ***out_types, size_t *out_count) {
    char **params = NULL;
    Type **types = NULL;
    size_t count = 0;
    size_t cap = 0;
    if (peek(p)->kind == TOK_RPAREN) {
        *out_params = NULL;
        *out_types = NULL;
        *out_count = 0;
        return 1;
    }
    while (1) {
        Type *type = parse_type(p);
        if (!type) { expect(p, TOK_INT, "expected type in parameter"); goto fail; }
        if (type->kind == TYPE_VOID) { free_type_local(type); if (!p->error) p->error = dup_error_at(peek(p), "void parameter not supported"); goto fail; }
        if (peek(p)->kind != TOK_IDENT) { expect(p, TOK_IDENT, "expected parameter name"); goto fail; }
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); goto fail; }
        advance(p);
        if (count + 1 > cap) {
            size_t next = (cap == 0) ? 4 : cap * 2;
            char **mem = (char **)realloc(params, next * sizeof(char *));
            if (!mem) { free(name); free_type_local(type); p->error = dup_error_at(peek(p), "out of memory"); goto fail; }
            params = mem;
            Type **tmem = (Type **)realloc(types, next * sizeof(Type *));
            if (!tmem) { free(name); free_type_local(type); p->error = dup_error_at(peek(p), "out of memory"); goto fail; }
            types = tmem;
            cap = next;
        }
        params[count++] = name;
        types[count - 1] = type;
        if (match(p, TOK_COMMA)) continue;
        break;
    }
    *out_params = params;
    *out_types = types;
    *out_count = count;
    return 1;

fail:
    for (size_t i = 0; i < count; i++) free(params[i]);
    free(params);
    if (types) {
        for (size_t i = 0; i < count; i++) free_type_local(types[i]);
    }
    free(types);
    return 0;
}

static Global *new_global(GlobalKind kind, char *name, long value, char *data, size_t len, Type *type) {
    Global *g = (Global *)calloc(1, sizeof(Global));
    if (!g) return NULL;
    g->kind = kind;
    g->name = name;
    g->value = value;
    g->data = data;
    g->len = len;
    g->type = type;
    return g;
}

static int program_add_global(Program *program, Global *g, Parser *p) {
    size_t next = program->global_count + 1;
    Global **mem = (Global **)realloc(program->globals, next * sizeof(Global *));
    if (!mem) {
        p->error = dup_error_at(peek(p), "out of memory");
        return 0;
    }
    program->globals = mem;
    program->globals[program->global_count++] = g;
    return 1;
}

Program *parse_program(const Token *tokens, size_t count, char **out_error) {
    Parser p = {0};
    p.tokens = tokens;
    p.count = count;
    p.pos = 0;
    p.error = NULL;
    p.structs = NULL;
    p.struct_count = 0;
    p.struct_cap = 0;
    Program *program = (Program *)calloc(1, sizeof(Program));
    if (!program) { if (out_error) *out_error = dup_error_at(peek(&p), "out of memory"); return NULL; }

    while (peek(&p)->kind != TOK_EOF) {
        Type *type = parse_type(&p);
        if (!type) { p.error = dup_error_at(peek(&p), "expected type"); goto done; }
        if ((type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) && match(&p, TOK_SEMI)) {
            free_type_local(type);
            continue;
        }
        if (peek(&p)->kind != TOK_IDENT) { expect(&p, TOK_IDENT, "expected name"); free_type_local(type); goto done; }
        char *name = dup_lexeme(peek(&p));
        if (!name) { free_type_local(type); p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
        advance(&p);

        if (match(&p, TOK_LBRACKET)) {
            if (type->kind != TYPE_INT) { free_type_local(type); free(name); p.error = dup_error_at(peek(&p), "only int arrays supported"); goto done; }
            if (peek(&p)->kind != TOK_NUMBER) { expect(&p, TOK_NUMBER, "expected array size"); free_type_local(type); free(name); goto done; }
            long count = peek(&p)->value;
            advance(&p);
            if (!expect(&p, TOK_RBRACKET, "expected ']'")) { free_type_local(type); free(name); goto done; }
            long *vals = NULL;
            size_t vcount = 0;
            if (match(&p, TOK_ASSIGN)) {
                if (!expect(&p, TOK_LBRACE, "expected '{'")) { free_type_local(type); free(name); goto done; }
                while (peek(&p)->kind != TOK_RBRACE && peek(&p)->kind != TOK_EOF) {
                    if (peek(&p)->kind != TOK_NUMBER) { expect(&p, TOK_NUMBER, "expected array element"); free(vals); free_type_local(type); free(name); goto done; }
                    long v = peek(&p)->value;
                    advance(&p);
                    long *mem = (long *)realloc(vals, (vcount + 1) * sizeof(long));
                    if (!mem) { p.error = dup_error_at(peek(&p), "out of memory"); free(vals); free_type_local(type); free(name); goto done; }
                    vals = mem;
                    vals[vcount++] = v;
                    if (match(&p, TOK_COMMA)) continue;
                    break;
                }
                if (!expect(&p, TOK_RBRACE, "expected '}'")) { free(vals); free_type_local(type); free(name); goto done; }
            }
            if (!expect(&p, TOK_SEMI, "expected ';'")) { free(vals); free_type_local(type); free(name); goto done; }
            Type *arr_type = new_type(TYPE_PTR, type);
            if (!arr_type) { free(vals); free_type_local(type); free(name); p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
            Global *g = new_global(GLOB_INT_ARR, name, count, (char *)vals, vcount, arr_type);
            if (!g) { p.error = dup_error_at(peek(&p), "out of memory"); free(vals); free_type_local(arr_type); free(name); goto done; }
            if (!program_add_global(program, g, &p)) { free(g->name); free(g->data); free(g->type); free(g); goto done; }
            continue;
        }

        if (match(&p, TOK_LPAREN)) {
            char **params = NULL;
            Type **param_types = NULL;
            size_t param_count = 0;
            if (!parse_params(&p, &params, &param_types, &param_count)) { free(name); free_type_local(type); goto done; }
            if (!expect(&p, TOK_RPAREN, "expected ')'") ) { free(name); free_type_local(type); goto done; }
            if (!expect(&p, TOK_LBRACE, "expected '{'") ) { free(name); free_type_local(type); goto done; }
            Stmt **stmts = NULL;
            size_t stmt_count = 0;
            if (!parse_stmt_list(&p, &stmts, &stmt_count)) { free(name); free_type_local(type); goto done; }
            if (!expect(&p, TOK_RBRACE, "expected '}'")) { free(name); free_type_local(type); goto done; }

            Function *func = new_function(name);
            if (!func) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
            func->params = params;
            func->param_types = param_types;
            func->param_count = param_count;
            func->stmts = stmts;
            func->stmt_count = stmt_count;
            func->ret_type = type;

            size_t next = program->func_count + 1;
            Function **mem = (Function **)realloc(program->funcs, next * sizeof(Function *));
            if (!mem) { p.error = dup_error_at(peek(&p), "out of memory"); free_function_local(func); goto done; }
            program->funcs = mem;
            program->funcs[program->func_count++] = func;
            continue;
        }

        if (type->kind == TYPE_VOID) { free_type_local(type); free(name); p.error = dup_error_at(peek(&p), "void variable not supported"); goto done; }
        if (type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) {
            free_type_local(type);
            free(name);
            p.error = dup_error_at(peek(&p), "struct/union globals not supported");
            goto done;
        }

        long value = 0;
        char *data = NULL;
        size_t len = 0;
        GlobalKind kind = GLOB_INT;
        if (match(&p, TOK_ASSIGN)) {
            if (peek(&p)->kind == TOK_STRING) {
                if (!(type->kind == TYPE_PTR && type->base && type->base->kind == TYPE_CHAR)) {
                    free_type_local(type);
                    free(name);
                    p.error = dup_error_at(peek(&p), "string literal requires char*");
                    goto done;
                }
                if (!decode_string(peek(&p), &data, &len, &p.error)) { free_type_local(type); free(name); goto done; }
                advance(&p);
                kind = GLOB_STR;
            } else if (peek(&p)->kind == TOK_NUMBER) {
                value = peek(&p)->value;
                advance(&p);
                kind = (type->kind == TYPE_CHAR) ? GLOB_CHAR : GLOB_INT;
            } else {
                expect(&p, TOK_NUMBER, "expected initializer");
                free_type_local(type);
                free(name);
                goto done;
            }
        } else {
            kind = (type->kind == TYPE_CHAR) ? GLOB_CHAR : GLOB_INT;
        }
        if (!expect(&p, TOK_SEMI, "expected ';'")) { free_type_local(type); free(name); free(data); goto done; }
        Global *g = new_global(kind, name, value, data, len, type);
        if (!g) { p.error = dup_error_at(peek(&p), "out of memory"); free_type_local(type); free(name); free(data); goto done; }
        if (!program_add_global(program, g, &p)) { free(g->name); free(g->data); free(g->type); free(g); goto done; }
        continue;
    }

done:
    program->structs = p.structs;
    program->struct_count = p.struct_count;
    if (p.error) {
        free_program(program);
    }
    if (out_error) *out_error = p.error;
    return p.error ? NULL : program;
}
