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
    if (stmt->cond) free_expr_local(stmt->cond);
    if (stmt->then_branch) free_stmt_local(stmt->then_branch);
    if (stmt->else_branch) free_stmt_local(stmt->else_branch);
    if (stmt->body) free_stmt_local(stmt->body);
    if (stmt->stmts) {
        for (size_t i = 0; i < stmt->stmt_count; i++) free_stmt_local(stmt->stmts[i]);
        free(stmt->stmts);
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

static Expr *parse_primary(Parser *p) {
    const Token *tok = peek(p);
    if (match(p, TOK_NUMBER)) {
        Expr *expr = new_expr(EXPR_NUMBER, tok->line, tok->col);
        if (!expr) return NULL;
        expr->as.number = tok->value;
        return expr;
    }
    if (match(p, TOK_IDENT)) {
        if (match(p, TOK_LPAREN)) {
            Expr **args = NULL;
            size_t count = 0;
            size_t cap = 0;
            if (peek(p)->kind != TOK_RPAREN) {
                while (1) {
                    Expr *arg = parse_expr(p);
                    if (!arg) {
                        for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                        free(args);
                        return NULL;
                    }
                    if (count + 1 > cap) {
                        size_t next = (cap == 0) ? 4 : cap * 2;
                        Expr **mem = (Expr **)realloc(args, next * sizeof(Expr *));
                        if (!mem) {
                            free_expr_local(arg);
                            for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                            free(args);
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
                return NULL;
            }
            Expr *expr = new_expr(EXPR_CALL, tok->line, tok->col);
            if (!expr) {
                for (size_t i = 0; i < count; i++) free_expr_local(args[i]);
                free(args);
                return NULL;
            }
            expr->as.call.name = dup_lexeme(tok);
            if (!expr->as.call.name) { free_expr_local(expr); return NULL; }
            expr->as.call.args = args;
            expr->as.call.arg_count = count;
            return expr;
        }
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

static Expr *parse_expr(Parser *p) {
    return parse_equality(p);
}

static Expr *parse_cond(Parser *p) {
    return parse_equality(p);
}

static int parse_stmt_list(Parser *p, Stmt ***out_stmts, size_t *out_count);

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
    if (match(p, TOK_RETURN)) {
        Expr *expr = parse_expr(p);
        if (!expr) return NULL;
        if (!expect(p, TOK_SEMI, "expected ';' after return")) { free_expr_local(expr); return NULL; }
        return new_stmt(STMT_RETURN, NULL, expr);
    }
    if (match(p, TOK_INT)) {
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
        char *name = dup_lexeme(peek(p));
        if (!name) { p->error = dup_error_at(peek(p), "out of memory"); return NULL; }
        advance(p);
        if (!expect(p, TOK_ASSIGN, "expected '='")) { free(name); return NULL; }
        Expr *value = parse_expr(p);
        if (!value) { free(name); return NULL; }
        if (!expect(p, TOK_SEMI, "expected ';' after assignment")) { free(name); free_expr_local(value); return NULL; }
        return new_stmt(STMT_ASSIGN, name, value);
    }
    if (!p->error) p->error = dup_error_at(peek(p), "expected statement");
    return NULL;
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

Program *parse_program(const Token *tokens, size_t count, char **out_error) {
    Parser p = {0};
    p.tokens = tokens;
    p.count = count;
    p.pos = 0;
    p.error = NULL;
    Program *program = (Program *)calloc(1, sizeof(Program));
    if (!program) { if (out_error) *out_error = dup_error_at(peek(&p), "out of memory"); return NULL; }

    while (peek(&p)->kind != TOK_EOF) {
        if (!expect(&p, TOK_INT, "expected 'int'")) goto done;
        if (peek(&p)->kind != TOK_IDENT) { expect(&p, TOK_IDENT, "expected function name"); goto done; }
        char *name = dup_lexeme(peek(&p));
        if (!name) { p.error = dup_error_at(peek(&p), "out of memory"); goto done; }
        advance(&p);
        if (!expect(&p, TOK_LPAREN, "expected '('") ) { free(name); goto done; }
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

        if (program->func_count + 1 > 0) {
            size_t next = program->func_count + 1;
            Function **mem = (Function **)realloc(program->funcs, next * sizeof(Function *));
            if (!mem) { p.error = dup_error_at(peek(&p), "out of memory"); free_function_local(func); goto done; }
            program->funcs = mem;
        }
        program->funcs[program->func_count++] = func;
    }

done:
    if (p.error) {
        free_program(program);
    }
    if (out_error) *out_error = p.error;
    return p.error ? NULL : program;
}
