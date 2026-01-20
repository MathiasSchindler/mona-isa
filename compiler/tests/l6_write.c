#include "clib.h"

int main() {
    char msg[2] = { 79, 75 };
    int n = write(1, msg, 2);
    if (n != 2) return 1;
    putchar(10);
    return 0;
}
