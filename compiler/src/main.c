#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"
#include "preproc.h"
#include "parser.h"
#include "ir.h"
#include "lower.h"
#include "codegen.h"
#include "opt.h"

static void print_usage(const char *prog) {
    fprintf(stderr, "usage: %s [--emit-ir] [--emit-asm] [--bin] [-O] [--max-errors N] [-o out.elf] <file.c>\n", prog);
}

static char *dup_string(const char *s) {
    size_t len = strlen(s) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}

static int ends_with(const char *s, const char *suffix) {
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    if (sl < su) return 0;
    return strcmp(s + (sl - su), suffix) == 0;
}

static int run_mina_as_with_mode(const char *as_path, const char *input_s, const char *output_path, int bin_out) {
    char cmd[1024];
    if (bin_out) snprintf(cmd, sizeof(cmd), "%s --bin --data-base 0 --bss-base 0 %s -o %s", as_path, input_s, output_path);
    else snprintf(cmd, sizeof(cmd), "%s %s -o %s", as_path, input_s, output_path);
    return system(cmd);
}

static char *write_temp_asm(const IRProgram *ir, char **out_error) {
#ifdef __EMSCRIPTEN__
    const char *tmp_path = "/tmp/minac-out.s";
    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        if (out_error) *out_error = dup_string("error: failed to open temp file");
        return NULL;
    }
    if (!codegen_emit_asm(ir, f, out_error)) {
        fclose(f);
        return NULL;
    }
    fclose(f);
    return dup_string(tmp_path);
#else
    char tmpl[] = "/tmp/minac-XXXXXX.s";
    int fd = mkstemps(tmpl, 2);
    if (fd < 0) {
        if (out_error) *out_error = dup_string("error: failed to create temp file");
        return NULL;
    }
    FILE *f = fdopen(fd, "w");
    if (!f) {
        close(fd);
        if (out_error) *out_error = dup_string("error: failed to open temp file");
        return NULL;
    }
    if (!codegen_emit_asm(ir, f, out_error)) {
        fclose(f);
        unlink(tmpl);
        return NULL;
    }
    fclose(f);
    return dup_string(tmpl);
#endif
}

int main(int argc, char **argv) {
    int emit_ir = 0;
    int emit_asm = 0;
    int emit_bin = 0;
    int optimize = 0;
    int max_errors = 5;
    const char *output_path = NULL;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--emit-ir") == 0) {
            emit_ir = 1;
        } else if (strcmp(argv[i], "--emit-asm") == 0) {
            emit_asm = 1;
        } else if (strcmp(argv[i], "--bin") == 0) {
            emit_bin = 1;
        } else if (strcmp(argv[i], "--max-errors") == 0) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            max_errors = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-O") == 0) {
            optimize = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            output_path = argv[++i];
        } else if (argv[i][0] == '-') {
            print_usage(argv[0]);
            return 1;
        } else {
            path = argv[i];
        }
    }

    if (!path || strlen(path) == 0) {
        print_usage(argv[0]);
        return 1;
    }
    if (emit_asm && output_path) {
        fprintf(stderr, "error: cannot use --emit-asm with -o\n");
        return 1;
    }
    if (emit_bin && !output_path) {
        fprintf(stderr, "error: --bin requires -o\n");
        return 1;
    }

    char *err = NULL;
    char *source = NULL;
    if (!preprocess_source(path, &source, &err)) {
        fprintf(stderr, "%s\n", err ? err : "error: preprocessor failed");
        free(err);
        return 1;
    }

    Token *tokens = NULL;
    size_t count = 0;
    if (!lex_all(source, &tokens, &count, &err)) {
        fprintf(stderr, "%s\n", err ? err : "error: lexing failed");
        free(err);
        free(source);
        return 1;
    }

    Program *program = parse_program(tokens, count, max_errors, &err);
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

    if (optimize) {
        ir_optimize(&ir);
    }

    if (emit_ir) {
        ir_print(&ir, stdout);
    }

    if (output_path) {
        char *asm_path = NULL;
        char *cg_err = NULL;
        asm_path = write_temp_asm(&ir, &cg_err);
        if (!asm_path) {
            fprintf(stderr, "%s\n", cg_err ? cg_err : "error: codegen failed");
            free(cg_err);
            ir_free(&ir);
            free_program(program);
            free_tokens(tokens);
            free(source);
            return 1;
        }
        const char *as_path = getenv("MINA_AS");
        if (!as_path) as_path = "../mina-as/mina-as";
        int bin_out = emit_bin || (output_path && ends_with(output_path, ".bin"));
        int rc = run_mina_as_with_mode(as_path, asm_path, output_path, bin_out);
        unlink(asm_path);
        free(asm_path);
        if (rc != 0) {
            fprintf(stderr, "error: mina-as failed\n");
            ir_free(&ir);
            free_program(program);
            free_tokens(tokens);
            free(source);
            return 1;
        }
    } else if (emit_asm || !emit_ir) {
        char *cg_err = NULL;
        if (!codegen_emit_asm(&ir, stdout, &cg_err)) {
            fprintf(stderr, "%s\n", cg_err ? cg_err : "error: codegen failed");
            free(cg_err);
            ir_free(&ir);
            free_program(program);
            free_tokens(tokens);
            free(source);
            return 1;
        }
    }

    ir_free(&ir);
    free_program(program);
    free_tokens(tokens);
    free(source);
    return 0;
}
