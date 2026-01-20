#ifndef MINAC_LOWER_H
#define MINAC_LOWER_H

#include "ast.h"
#include "ir.h"

int lower_program(const Program *program, IRProgram *ir, int prefer_libc, char **out_error);

#endif
