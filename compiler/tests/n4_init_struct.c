struct Pair {
    int left;
    char tag;
    int right;
};

union Cell {
    int word;
    char byte;
};

struct Pair gp = { 1, 65, 3 };
union Cell gu = { 90 };
int garr[3] = { 4, 5, 6 };

int main() {
    struct Pair p = { 7, 66, 8 };
    union Cell u = { 91 };
    int arr[4] = { 1, 2, 3, 4 };

    int sum = p.left + p.tag + p.right + gp.left + gp.tag + gp.right + garr[0] + garr[1] + garr[2];

    if (u.word != 91) {
        putchar(70);
        return 1;
    }
    if (gu.word != 90) {
        putchar(70);
        return 1;
    }
    if (arr[3] != 4) {
        putchar(70);
        return 1;
    }
    if (sum != 165) {
        putchar(70);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
