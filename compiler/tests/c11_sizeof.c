int main() {
    int a[3];
    char c;
    int *p = a;

    if (sizeof(a) != 24) {
        putchar(70);
        putchar(65);
        putchar(73);
        putchar(76);
        return 1;
    }
    if (sizeof(a[0]) != 8) {
        putchar(70);
        putchar(65);
        putchar(73);
        putchar(76);
        return 1;
    }
    if (sizeof(p) != 8) {
        putchar(70);
        putchar(65);
        putchar(73);
        putchar(76);
        return 1;
    }
    if (sizeof(c) != 1) {
        putchar(70);
        putchar(65);
        putchar(73);
        putchar(76);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
