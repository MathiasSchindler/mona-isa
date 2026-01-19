#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_error(const char *msg, int line, int col) {
    char buf[256];
    snprintf(buf, sizeof(buf), "error: %d:%d: %s", line, col, msg);
    size_t len = strlen(buf) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, buf, len);
    return out;
}

static int push_token(Token **tokens, size_t *count, size_t *cap, Token tok) {
    if (*count + 1 > *cap) {
        size_t next = (*cap == 0) ? 64 : (*cap * 2);
        Token *mem = (Token *)realloc(*tokens, next * sizeof(Token));
        if (!mem) return 0;
        *tokens = mem;
        *cap = next;
    }
    (*tokens)[(*count)++] = tok;
    return 1;
}

static int is_ident_start(int c) {
    return isalpha(c) || c == '_';
}

static int is_ident_cont(int c) {
    return isalnum(c) || c == '_';
}

int lex_all(const char *src, Token **out_tokens, size_t *out_count, char **out_error) {
    Token *tokens = NULL;
    size_t count = 0, cap = 0;
    int line = 1, col = 1;

    for (size_t i = 0; src[i] != '\0';) {
        char c = src[i];
        if (c == ' ' || c == '\t' || c == '\r') {
            i++; col++;
            continue;
        }
        if (c == '\n') {
            i++; line++; col = 1;
            continue;
        }
        if (c == '/' && src[i + 1] == '/') {
            i += 2; col += 2;
            while (src[i] && src[i] != '\n') { i++; col++; }
            continue;
        }
        if (c == '/' && src[i + 1] == '*') {
            i += 2; col += 2;
            while (src[i]) {
                if (src[i] == '*' && src[i + 1] == '/') { i += 2; col += 2; break; }
                if (src[i] == '\n') { i++; line++; col = 1; continue; }
                i++; col++;
            }
            continue;
        }

        Token tok = {0};
        tok.lexeme = &src[i];
        tok.line = line;
        tok.col = col;

        if (isdigit((unsigned char)c)) {
            long v = 0;
            size_t start = i;
            while (isdigit((unsigned char)src[i])) {
                v = v * 10 + (src[i] - '0');
                i++; col++;
            }
            tok.kind = TOK_NUMBER;
            tok.length = (int)(i - start);
            tok.value = v;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }

        if (is_ident_start((unsigned char)c)) {
            size_t start = i;
            while (is_ident_cont((unsigned char)src[i])) { i++; col++; }
            tok.length = (int)(i - start);
            tok.kind = TOK_IDENT;
            if (tok.length == 3 && strncmp(tok.lexeme, "int", 3) == 0) tok.kind = TOK_INT;
            if (tok.length == 4 && strncmp(tok.lexeme, "char", 4) == 0) tok.kind = TOK_CHAR;
            if (tok.length == 3 && strncmp(tok.lexeme, "for", 3) == 0) tok.kind = TOK_FOR;
            if (tok.length == 6 && strncmp(tok.lexeme, "switch", 6) == 0) tok.kind = TOK_SWITCH;
            if (tok.length == 4 && strncmp(tok.lexeme, "case", 4) == 0) tok.kind = TOK_CASE;
            if (tok.length == 7 && strncmp(tok.lexeme, "default", 7) == 0) tok.kind = TOK_DEFAULT;
            if (tok.length == 5 && strncmp(tok.lexeme, "break", 5) == 0) tok.kind = TOK_BREAK;
            if (tok.length == 8 && strncmp(tok.lexeme, "continue", 8) == 0) tok.kind = TOK_CONTINUE;
            if (tok.length == 6 && strncmp(tok.lexeme, "return", 6) == 0) tok.kind = TOK_RETURN;
            if (tok.length == 2 && strncmp(tok.lexeme, "if", 2) == 0) tok.kind = TOK_IF;
            if (tok.length == 4 && strncmp(tok.lexeme, "else", 4) == 0) tok.kind = TOK_ELSE;
            if (tok.length == 5 && strncmp(tok.lexeme, "while", 5) == 0) tok.kind = TOK_WHILE;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }

        if (c == '"') {
            size_t start = i;
            i++; col++;
            while (src[i] && src[i] != '"') {
                if (src[i] == '\\' && src[i + 1]) { i += 2; col += 2; continue; }
                if (src[i] == '\n') { i++; line++; col = 1; continue; }
                i++; col++;
            }
            if (src[i] != '"') {
                if (out_error) *out_error = dup_error("unterminated string literal", line, col);
                free(tokens);
                return 0;
            }
            i++; col++;
            tok.kind = TOK_STRING;
            tok.lexeme = &src[start];
            tok.length = (int)(i - start);
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }

        if (c == '=' && src[i + 1] == '=') {
            tok.kind = TOK_EQ;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '&' && src[i + 1] == '&') {
            tok.kind = TOK_ANDAND;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '|' && src[i + 1] == '|') {
            tok.kind = TOK_OROR;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '!' && src[i + 1] == '=') {
            tok.kind = TOK_NEQ;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '!' ) {
            tok.kind = TOK_NOT;
            tok.length = 1;
            i++; col++;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '<' && src[i + 1] == '=') {
            tok.kind = TOK_LTE;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }
        if (c == '>' && src[i + 1] == '=') {
            tok.kind = TOK_GTE;
            tok.length = 2;
            i += 2; col += 2;
            if (!push_token(&tokens, &count, &cap, tok)) goto oom;
            continue;
        }

        switch (c) {
            case '(' : tok.kind = TOK_LPAREN; tok.length = 1; break;
            case ')' : tok.kind = TOK_RPAREN; tok.length = 1; break;
            case '{' : tok.kind = TOK_LBRACE; tok.length = 1; break;
            case '}' : tok.kind = TOK_RBRACE; tok.length = 1; break;
            case ';' : tok.kind = TOK_SEMI; tok.length = 1; break;
            case ',' : tok.kind = TOK_COMMA; tok.length = 1; break;
            case '+' : tok.kind = TOK_PLUS; tok.length = 1; break;
            case '-' : tok.kind = TOK_MINUS; tok.length = 1; break;
            case '*' : tok.kind = TOK_STAR; tok.length = 1; break;
            case '/' : tok.kind = TOK_SLASH; tok.length = 1; break;
            case '=' : tok.kind = TOK_ASSIGN; tok.length = 1; break;
            case '<' : tok.kind = TOK_LT; tok.length = 1; break;
            case '>' : tok.kind = TOK_GT; tok.length = 1; break;
            case '&' : tok.kind = TOK_AMP; tok.length = 1; break;
            case ':' : tok.kind = TOK_COLON; tok.length = 1; break;
            case '[' : tok.kind = TOK_LBRACKET; tok.length = 1; break;
            case ']' : tok.kind = TOK_RBRACKET; tok.length = 1; break;
            default:
                if (out_error) *out_error = dup_error("unexpected character", line, col);
                free(tokens);
                return 0;
        }
        i++; col++;
        if (!push_token(&tokens, &count, &cap, tok)) goto oom;
    }

    Token eof = {0};
    eof.kind = TOK_EOF;
    eof.lexeme = "";
    eof.length = 0;
    eof.line = line;
    eof.col = col;
    if (!push_token(&tokens, &count, &cap, eof)) goto oom;

    *out_tokens = tokens;
    *out_count = count;
    if (out_error) *out_error = NULL;
    return 1;

oom:
    if (out_error) *out_error = dup_error("out of memory", line, col);
    free(tokens);
    return 0;
}

void free_tokens(Token *tokens) {
    free(tokens);
}
