struct Pair {
    int left;
    char tag;
    int right;
};

int main() {
    struct Pair p;
    struct Pair *pp = &p;

    p.left = 11;
    p.tag = 65;
    p.right = 22;

    if (pp->left != 11) {
        putchar(70);
        return 1;
    }
    if (pp->tag != 65) {
        putchar(70);
        return 1;
    }
    if (pp->right != 22) {
        putchar(70);
        return 1;
    }

    putchar(79);
    putchar(75);
    return 0;
}
