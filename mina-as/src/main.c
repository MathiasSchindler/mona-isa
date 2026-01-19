#include "asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = "a.out.elf";
    bool elf_output = true;
    AsmOptions opt = {
        .text_base = 0x0,
        .data_base = 0x1000,
        .bss_base = 0x2000,
        .seg_align = 0x1000,
        .optimize = false,
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--elf") == 0 || strcmp(argv[i], "-elf") == 0) {
            elf_output = true;
        } else if (strcmp(argv[i], "--bin") == 0 || strcmp(argv[i], "-bin") == 0) {
            elf_output = false;
        } else if (strcmp(argv[i], "--text-base") == 0 && i + 1 < argc) {
            opt.text_base = strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--data-base") == 0 && i + 1 < argc) {
            opt.data_base = strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--bss-base") == 0 && i + 1 < argc) {
            opt.bss_base = strtoull(argv[++i], NULL, 0);
        } else if ((strcmp(argv[i], "--segment-align") == 0 || strcmp(argv[i], "--seg-align") == 0) && i + 1 < argc) {
            opt.seg_align = strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--optimize") == 0) {
            opt.optimize = true;
        } else if (!in_path) {
            in_path = argv[i];
        } else {
            fprintf(stderr, "usage: %s [--elf|--bin] [--text-base N] [--data-base N] [--bss-base N] [--segment-align N] input.s -o output.elf\n", argv[0]);
            return 1;
        }
    }

    if (!in_path) {
        fprintf(stderr, "usage: %s [--elf|--bin] [--text-base N] [--data-base N] [--bss-base N] [--segment-align N] input.s -o output.elf\n", argv[0]);
        return 1;
    }

    if (elf_output && ends_with(out_path, ".bin")) elf_output = false;

    if (!assemble_file(in_path, out_path, elf_output, &opt)) {
        fprintf(stderr, "assembly failed\n");
        return 1;
    }

    return 0;
}
