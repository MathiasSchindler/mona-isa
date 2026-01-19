#ifndef MINAC_LEXER_H
#define MINAC_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_INT,
    TOK_RETURN,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMI,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *lexeme;
    int length;
    int line;
    int col;
    long value;
} Token;

int lex_all(const char *src, Token **out_tokens, size_t *out_count, char **out_error);
void free_tokens(Token *tokens);

#endif
