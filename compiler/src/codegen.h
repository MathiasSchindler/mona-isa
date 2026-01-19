#ifndef MINAC_CODEGEN_H
#define MINAC_CODEGEN_H

#include <stdio.h>
#include "ir.h"

int codegen_emit_asm(const IRProgram *ir, FILE *out, int emit_start, char **out_error);

#endif
