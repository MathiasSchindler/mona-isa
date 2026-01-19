#include "include/c13_header.h"

int main() {
    int v = inc(MAGIC);
    if (v != 42) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
