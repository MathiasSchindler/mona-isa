struct Item {
    int value;
    char tag;
};

int sum_items(struct Item *items, int n) {
    int i = 0;
    int sum = 0;
    for (i = 0; i < n; i = i + 1) {
        sum = sum + items[i].value;
    }
    return sum;
}

int main() {
    struct Item items[4];
    items[0].value = 10;
    items[0].tag = 65;
    items[1].value = 20;
    items[1].tag = 66;
    items[2].value = 30;
    items[2].tag = 67;
    items[3].value = 40;
    items[3].tag = 68;

    int sum = sum_items(items, 4);
    if (sum != 100) {
        putchar(70);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
