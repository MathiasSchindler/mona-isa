int main() {
    char buf[8];
    int i = 0;
    int sum = 0;
    for (i = 0; i < 8; i = i + 1) {
        buf[i] = (i + 1) * 3;
    }
    for (i = 0; i < 8; i = i + 1) {
        sum = sum + buf[i];
    }
    if (sum != 108) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
