#define A 6
#define B 7

int main() {
    int x = A * B;
    if (x != 42) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
