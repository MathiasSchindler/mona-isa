#include "clib.h"

int main() {
    int *p = malloc(16);
    if (!p) return 1;
    putchar(79);
    putchar(75);
    putchar(10);
    free(p);
    return 0;
}
