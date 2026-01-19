union Cell {
    int word;
    char byte;
};

int main() {
    union Cell cells[3];
    int i = 0;
    for (i = 0; i < 3; i = i + 1) {
        cells[i].word = 0;
    }
    cells[0].byte = 5;
    cells[1].byte = 7;
    cells[2].byte = 9;

    int sum = 0;
    for (i = 0; i < 3; i = i + 1) {
        sum = sum + cells[i].word;
    }

    if (sum != 21) {
        putchar(70);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
