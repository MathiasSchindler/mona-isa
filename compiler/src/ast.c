#include "ast.h"
#include <stdlib.h>

static void free_type(Type *type) {
    if (!type) return;
    if (type->base) free_type(type->base);
    free(type);
}

static void free_struct_def(StructDef *def) {
    if (!def) return;
    free(def->name);
    if (def->fields) {
        for (size_t i = 0; i < def->field_count; i++) {
            free(def->fields[i].name);
            if (def->fields[i].type) free_type(def->fields[i].type);
        }
        free(def->fields);
    }
    free(def);
}

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
        case EXPR_STRING:
            free(expr->as.str.data);
            break;
        case EXPR_INDEX:
            free_expr(expr->as.index.base);
            free_expr(expr->as.index.index);
            break;
        case EXPR_FIELD:
            free_expr(expr->as.field.base);
            free(expr->as.field.field);
            break;
        case EXPR_SIZEOF:
            if (expr->as.sizeof_expr.type) free_type(expr->as.sizeof_expr.type);
            if (expr->as.sizeof_expr.expr) free_expr(expr->as.sizeof_expr.expr);
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
    if (stmt->lhs) free_expr(stmt->lhs);
    if (stmt->index) free_expr(stmt->index);
    if (stmt->cond) free_expr(stmt->cond);
    if (stmt->then_branch) free_stmt(stmt->then_branch);
    if (stmt->else_branch) free_stmt(stmt->else_branch);
    if (stmt->body) free_stmt(stmt->body);
    if (stmt->init) free_stmt(stmt->init);
    if (stmt->post) free_stmt(stmt->post);
    if (stmt->decl_type) free_type(stmt->decl_type);
    if (stmt->stmts) {
        for (size_t i = 0; i < stmt->stmt_count; i++) free_stmt(stmt->stmts[i]);
        free(stmt->stmts);
    }
    if (stmt->cases) {
        for (size_t i = 0; i < stmt->case_count; i++) {
            for (size_t j = 0; j < stmt->cases[i].stmt_count; j++) free_stmt(stmt->cases[i].stmts[j]);
            free(stmt->cases[i].stmts);
        }
        free(stmt->cases);
    }
    if (stmt->default_stmts) {
        for (size_t i = 0; i < stmt->default_count; i++) free_stmt(stmt->default_stmts[i]);
        free(stmt->default_stmts);
    }
    free(stmt);
}

static void free_function(Function *func) {
    if (!func) return;
    free(func->name);
    for (size_t i = 0; i < func->param_count; i++) free(func->params[i]);
    free(func->params);
    if (func->param_types) {
        for (size_t i = 0; i < func->param_count; i++) free_type(func->param_types[i]);
        free(func->param_types);
    }
    if (func->ret_type) free_type(func->ret_type);
    for (size_t i = 0; i < func->stmt_count; i++) {
        free_stmt(func->stmts[i]);
    }
    free(func->stmts);
    free(func);
}

void free_program(Program *program) {
    if (!program) return;
    if (program->structs) {
        for (size_t i = 0; i < program->struct_count; i++) {
            free_struct_def(program->structs[i]);
        }
        free(program->structs);
    }
    for (size_t i = 0; i < program->func_count; i++) {
        free_function(program->funcs[i]);
    }
    free(program->funcs);
    for (size_t i = 0; i < program->global_count; i++) {
        Global *g = program->globals[i];
        if (!g) continue;
        free(g->name);
        free(g->data);
        if (g->type) free_type(g->type);
        free(g);
    }
    free(program->globals);
    free(program);
}
