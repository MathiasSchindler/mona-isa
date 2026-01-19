#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const Token *tokens;
    size_t count;
    size_t pos;
    char *error;
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

static Expr *new_expr(ExprKind kind, int line, int col) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (!expr) return NULL;
    expr->kind = kind;
    expr->line = line;
    expr->col = col;
    return expr;
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
        break;
    }
    return expr;
}

static Expr *parse_unary(Parser *p) {
    const Token *tok = peek(p);
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
    if (expr->kind == EXPR_IDENT || expr->kind == EXPR_INDEX) return 1;
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
            if (match(p, TOK_INT)) {
                while (match(p, TOK_STAR)) { }
                if (peek(p)->kind != TOK_IDENT) { expect(p, TOK_IDENT, "expected identifier"); return NULL; }
                char *name = dup_lexeme(peek(p));
                if (!name) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
                advance(p);
                Expr *val = NULL;
                if (match(p, TOK_ASSIGN)) {
                    val = parse_expr(p);
                    if (!val) { free(name); return NULL; }
                }
                init = new_stmt(STMT_DECL, name, val);
                if (!init) { free(name); free_expr_local(val); return NULL; }
            } else if (peek(p)->kind == TOK_IDENT && (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET)) {
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
            if (peek(p)->kind == TOK_IDENT && (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET)) {
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
        Expr *expr = parse_expr(p);
        if (!expr) return NULL;
        if (!expect(p, TOK_SEMI, "expected ';' after return")) { free_expr_local(expr); return NULL; }
        return new_stmt(STMT_RETURN, NULL, expr);
    }
    if (match(p, TOK_INT)) {
        while (match(p, TOK_STAR)) { }
        if (peek(p)->kind != TOK_IDENT) {
            expect(p, TOK_IDENT, "expected identifier");
            return NULL;
        }
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
        advance(p);
        Expr *init = NULL;
        if (match(p, TOK_ASSIGN)) {
            init = parse_expr(p);
            if (!init) { free(name); return NULL; }
        }
        if (!expect(p, TOK_SEMI, "expected ';' after declaration")) { free(name); free_expr_local(init); return NULL; }
        return new_stmt(STMT_DECL, name, init);
    }
    if (peek(p)->kind == TOK_IDENT) {
        if (peek_n(p, 1)->kind == TOK_ASSIGN || peek_n(p, 1)->kind == TOK_LBRACKET) {
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

static int parse_params(Parser *p, char ***out_params, size_t *out_count) {
    char **params = NULL;
    size_t count = 0;
    size_t cap = 0;
    if (peek(p)->kind == TOK_RPAREN) {
        *out_params = NULL;
        *out_count = 0;
        return 1;
    }
    while (1) {
        if (!expect(p, TOK_INT, "expected 'int' in parameter")) goto fail;
        while (match(p, TOK_STAR)) { }
        if (peek(p)->kind != TOK_IDENT) { expect(p, TOK_IDENT, "expected parameter name"); goto fail; }
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); goto fail; }
        advance(p);
        if (count + 1 > cap) {
            size_t next = (cap == 0) ? 4 : cap * 2;
            char **mem = (char **)realloc(params, next * sizeof(char *));
            if (!mem) { free(name); p->error = dup_error_at(peek(p), "out of memory"); goto fail; }
            params = mem;
            cap = next;
        }
        params[count++] = name;
        if (match(p, TOK_COMMA)) continue;
        break;
    }
    *out_params = params;
    *out_count = count;
    return 1;

fail:
    for (size_t i = 0; i < count; i++) free(params[i]);
    free(params);
    return 0;
}

static Global *new_global(GlobalKind kind, char *name, long value, char *data, size_t len) {
    Global *g = (Global *)calloc(1, sizeof(Global));
    if (!g) return NULL;
    g->kind = kind;
    g->name = name;
    g->value = value;
    g->data = data;
    g->len = len;
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
    Program *program = (Program *)calloc(1, sizeof(Program));
    if (!program) { if (out_error) *out_error = dup_error_at(peek(&p), "out of memory"); return NULL; }

    while (peek(&p)->kind != TOK_EOF) {
        if (match(&p, TOK_INT)) {
            while (match(&p, TOK_STAR)) { }
            if (peek(&p)->kind != TOK_IDENT) { expect(&p, TOK_IDENT, "expected name"); goto done; }
            char *name = dup_lexeme(peek(&p));
            if (!name) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
            advance(&p);
            if (match(&p, TOK_LBRACKET)) {
                if (peek(&p)->kind != TOK_NUMBER) { expect(&p, TOK_NUMBER, "expected array size"); free(name); goto done; }
                long count = peek(&p)->value;
                advance(&p);
                if (!expect(&p, TOK_RBRACKET, "expected ']'")) { free(name); goto done; }
                long *vals = NULL;
                size_t vcount = 0;
                if (match(&p, TOK_ASSIGN)) {
                    if (!expect(&p, TOK_LBRACE, "expected '{'")) { free(name); goto done; }
                    while (peek(&p)->kind != TOK_RBRACE && peek(&p)->kind != TOK_EOF) {
                        if (peek(&p)->kind != TOK_NUMBER) { expect(&p, TOK_NUMBER, "expected array element"); free(vals); free(name); goto done; }
                        long v = peek(&p)->value;
                        advance(&p);
                        long *mem = (long *)realloc(vals, (vcount + 1) * sizeof(long));
                        if (!mem) { p.error = dup_error_at(peek(&p), "out of memory"); free(vals); free(name); goto done; }
                        vals = mem;
                        vals[vcount++] = v;
                        if (match(&p, TOK_COMMA)) continue;
                        break;
                    }
                    if (!expect(&p, TOK_RBRACE, "expected '}'")) { free(vals); free(name); goto done; }
                }
                if (!expect(&p, TOK_SEMI, "expected ';'")) { free(vals); free(name); goto done; }
                Global *g = new_global(GLOB_INT_ARR, name, count, (char *)vals, vcount);
                if (!g) { p.error = dup_error_at(peek(&p), "out of memory"); free(vals); free(name); goto done; }
                if (!program_add_global(program, g, &p)) { free(g->name); free(g->data); free(g); goto done; }
            } else if (match(&p, TOK_LPAREN)) {
                char **params = NULL;
                size_t param_count = 0;
                if (!parse_params(&p, &params, &param_count)) { free(name); goto done; }
                if (!expect(&p, TOK_RPAREN, "expected ')'") ) { free(name); goto done; }
                if (!expect(&p, TOK_LBRACE, "expected '{'") ) { free(name); goto done; }
                Stmt **stmts = NULL;
                size_t stmt_count = 0;
                if (!parse_stmt_list(&p, &stmts, &stmt_count)) { free(name); goto done; }
                if (!expect(&p, TOK_RBRACE, "expected '}'")) { free(name); goto done; }

                Function *func = new_function(name);
                if (!func) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
                func->params = params;
                func->param_count = param_count;
                func->stmts = stmts;
                func->stmt_count = stmt_count;

                size_t next = program->func_count + 1;
                Function **mem = (Function **)realloc(program->funcs, next * sizeof(Function *));
                if (!mem) { p.error = dup_error_at(peek(&p), "out of memory"); free_function_local(func); goto done; }
                program->funcs = mem;
                program->funcs[program->func_count++] = func;
            } else {
                long value = 0;
                if (match(&p, TOK_ASSIGN)) {
                    if (peek(&p)->kind != TOK_NUMBER) { expect(&p, TOK_NUMBER, "expected integer literal"); free(name); goto done; }
                    value = peek(&p)->value;
                    advance(&p);
                }
                if (!expect(&p, TOK_SEMI, "expected ';'")) { free(name); goto done; }
                Global *g = new_global(GLOB_INT, name, value, NULL, 0);
                if (!g) { p.error = dup_error_at(peek(&p), "out of memory"); free(name); goto done; }
                if (!program_add_global(program, g, &p)) { free(g->name); free(g); goto done; }
            }
            continue;
        }
        if (match(&p, TOK_CHAR)) {
            if (!expect(&p, TOK_STAR, "expected '*'") ) goto done;
            if (peek(&p)->kind != TOK_IDENT) { expect(&p, TOK_IDENT, "expected name"); goto done; }
            char *name = dup_lexeme(peek(&p));
            if (!name) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
            advance(&p);
            if (!expect(&p, TOK_ASSIGN, "expected '='")) { free(name); goto done; }
            if (peek(&p)->kind != TOK_STRING) { expect(&p, TOK_STRING, "expected string literal"); free(name); goto done; }
            char *data = NULL;
            size_t len = 0;
            if (!decode_string(peek(&p), &data, &len, &p.error)) { free(name); goto done; }
            advance(&p);
            if (!expect(&p, TOK_SEMI, "expected ';'")) { free(name); free(data); goto done; }
            Global *g = new_global(GLOB_STR, name, 0, data, len);
            if (!g) { p.error = dup_error_at(peek(&p), "out of memory"); free(name); free(data); goto done; }
            if (!program_add_global(program, g, &p)) { free(g->name); free(g->data); free(g); goto done; }
            continue;
        }
        p.error = dup_error_at(peek(&p), "expected top-level declaration");
        goto done;
    }

done:
    if (p.error) {
        free_program(program);
    }
    if (out_error) *out_error = p.error;
    return p.error ? NULL : program;
}
