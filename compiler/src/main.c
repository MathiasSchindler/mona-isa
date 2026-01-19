#include <stdio.h>
#include <string.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "usage: %s <file.c>\n", prog);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *path = argv[1];
    if (strlen(path) == 0) {
        print_usage(argv[0]);
        return 1;
    }

    printf("minac: not implemented yet (input: %s)\n", path);
    return 0;
}
