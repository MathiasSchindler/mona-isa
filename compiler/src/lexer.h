#ifndef MINAC_LEXER_H
#define MINAC_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_INT,
    TOK_CHAR,
    TOK_VOID,
    TOK_FOR,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_SIZEOF,
    TOK_STRUCT,
    TOK_UNION,
    TOK_ENUM,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMI,
    TOK_COMMA,
    TOK_COLON,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_DOT,
    TOK_ARROW,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_ASSIGN,
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE,
    TOK_STRING,
    TOK_AMP,
    TOK_ANDAND,
    TOK_OROR,
    TOK_NOT
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *lexeme;
    int length;
    int line;
    int col;
    const char *line_start;
    long value;
} Token;

int lex_all(const char *src, Token **out_tokens, size_t *out_count, char **out_error);
void free_tokens(Token *tokens);

#endif
