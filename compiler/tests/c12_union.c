union Cell {
    int word;
    char byte;
};

int main() {
    union Cell c;
    c.word = 0;
    c.byte = 90;

    if (c.byte != 90) {
        putchar(70);
        return 1;
    }
    if (c.word != 90) {
        putchar(70);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
