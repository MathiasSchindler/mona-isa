#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "lower.h"

static void print_usage(const char *prog) {
    fprintf(stderr, "usage: %s [--emit-ir] <file.c>\n", prog);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (n != (size_t)len) { free(buf); return NULL; }
    buf[len] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    int emit_ir = 0;
    const char *path = NULL;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--emit-ir") == 0) {
        emit_ir = 1;
        path = argv[2];
    } else {
        print_usage(argv[0]);
        return 1;
    }
    if (strlen(path) == 0) {
        print_usage(argv[0]);
        return 1;
    }

    char *source = read_file(path);
    if (!source) {
        fprintf(stderr, "error: failed to read '%s'\n", path);
        return 1;
    }

    Token *tokens = NULL;
    size_t count = 0;
    char *err = NULL;
    if (!lex_all(source, &tokens, &count, &err)) {
        fprintf(stderr, "%s\n", err ? err : "error: lexing failed");
        free(err);
        free(source);
        return 1;
    }

    Program *program = parse_program(tokens, count, &err);
    if (!program) {
        fprintf(stderr, "%s\n", err ? err : "error: parse failed");
        free(err);
        free_tokens(tokens);
        free(source);
        return 1;
    }

    IRProgram ir;
    ir_init(&ir);
    if (!lower_program(program, &ir, &err)) {
        fprintf(stderr, "%s\n", err ? err : "error: lower failed");
        free(err);
        ir_free(&ir);
        free_program(program);
        free_tokens(tokens);
        free(source);
        return 1;
    }

    if (emit_ir) {
        ir_print(&ir, stdout);
    } else {
        printf("minac: parse ok\n");
    }

    ir_free(&ir);
    free_program(program);
    free_tokens(tokens);
    free(source);
    return 0;
}
