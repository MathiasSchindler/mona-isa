int g = 2 + 3 * 4;
char c = 1 + 2;

int main() {
    int x = (5 + 5) * 3;
    if (g != 14) {
        putchar(70);
        return 1;
    }
    if (c != 3) {
        putchar(70);
        return 1;
    }
    if (x != 30) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
