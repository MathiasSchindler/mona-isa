enum Color { RED = 1, GREEN, BLUE = 10, WHITE };

enum Color c = GREEN;

int main() {
    int v = RED + GREEN + BLUE + WHITE;
    if (c != 2) {
        putchar(70);
        return 1;
    }
    switch (v) {
        case 24:
            putchar(79);
            putchar(75);
            return 0;
        default:
            putchar(70);
            return 1;
    }
}
