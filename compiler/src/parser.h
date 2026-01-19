#ifndef MINAC_PARSER_H
#define MINAC_PARSER_H

#include <stddef.h>
#include "lexer.h"
#include "ast.h"

Program *parse_program(const Token *tokens, size_t count, int max_errors, char **out_error);

#endif
