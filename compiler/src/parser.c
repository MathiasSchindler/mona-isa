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
        case EXPR_NUMBER:
        default:
            break;
    }
    free(expr);
}

static Stmt *new_stmt_return(Expr *expr) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) return NULL;
    stmt->kind = STMT_RETURN;
    stmt->expr = expr;
    return stmt;
}

static void free_stmt_local(Stmt *stmt) {
    if (!stmt) return;
    if (stmt->kind == STMT_RETURN) free_expr_local(stmt->expr);
    free(stmt);
}

static Function *new_function(char *name, Stmt *body) {
    Function *fn = (Function *)calloc(1, sizeof(Function));
    if (!fn) return NULL;
    fn->name = name;
    fn->body = body;
    return fn;
}

static Program *new_program(Function *func) {
    Program *p = (Program *)calloc(1, sizeof(Program));
    if (!p) return NULL;
    p->func = func;
    return p;
}

static Expr *parse_expr(Parser *p);

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
    if (match(p, TOK_LPAREN)) {
        Expr *inner = parse_expr(p);
        if (!inner) return NULL;
        if (!expect(p, TOK_RPAREN, "expected ')'") ) { free_expr_local(inner); return NULL; }
        return inner;
    }
    if (!p->error) p->error = dup_error_at(peek(p), "expected primary expression");
    return NULL;
}

static Expr *parse_unary(Parser *p) {
    const Token *tok = peek(p);
    if (match(p, TOK_PLUS) || match(p, TOK_MINUS)) {
        Expr *rhs = parse_unary(p);
        if (!rhs) return NULL;
        Expr *expr = new_expr(EXPR_UNARY, tok->line, tok->col);
        if (!expr) { free_expr_local(rhs); return NULL; }
        expr->as.unary.op = (tok->kind == TOK_MINUS) ? UN_MINUS : UN_PLUS;
        expr->as.unary.expr = rhs;
        return expr;
    }
    return parse_primary(p);
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

static Expr *parse_expr(Parser *p) {
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

static Stmt *parse_stmt(Parser *p) {
    if (match(p, TOK_RETURN)) {
        Expr *expr = parse_expr(p);
        if (!expr) return NULL;
        if (!expect(p, TOK_SEMI, "expected ';' after return")) { free_expr_local(expr); return NULL; }
        return new_stmt_return(expr);
    }
    if (!p->error) p->error = dup_error_at(peek(p), "expected 'return' statement");
    return NULL;
}

Program *parse_program(const Token *tokens, size_t count, char **out_error) {
    Parser p = {0};
    p.tokens = tokens;
    p.count = count;
    p.pos = 0;
    p.error = NULL;
    Program *program = NULL;
    Function *func = NULL;
    Stmt *body = NULL;
    char *name = NULL;

    if (!expect(&p, TOK_INT, "expected 'int'")) goto done;
    if (peek(&p)->kind != TOK_IDENT) {
        expect(&p, TOK_IDENT, "expected function name");
        goto done;
    }
    name = dup_lexeme(peek(&p));
    if (!name) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
    advance(&p);
    if (!expect(&p, TOK_LPAREN, "expected '('") ) goto done;
    if (!expect(&p, TOK_RPAREN, "expected ')'") ) goto done;
    if (!expect(&p, TOK_LBRACE, "expected '{'") ) goto done;
    body = parse_stmt(&p);
    if (!body) goto done;
    if (!expect(&p, TOK_RBRACE, "expected '}'")) goto done;
    if (!expect(&p, TOK_EOF, "expected end of file")) goto done;

    func = new_function(name, body);
    if (!func) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
    program = new_program(func);
    if (!program) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }

done:
    if (p.error) {
        if (name && !func) free(name);
        if (body && !func) free_stmt_local(body);
        if (func && !program) { free(func->name); free_stmt_local(func->body); free(func); }
        free_program(program);
    }
    if (out_error) *out_error = p.error;
    return p.error ? NULL : program;
}
