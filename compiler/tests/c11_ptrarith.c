int main() {
    int a[4];
    int *p = a;

    *(p + 0) = 7;
    *(p + 1) = 8;
    *(p + 2) = 9;

    int x = *(p + 1) + *(p + 2);
    if (x != 17) {
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
