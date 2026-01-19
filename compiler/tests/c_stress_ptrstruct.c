struct Node {
    int value;
    struct Node *next;
};

int main() {
    struct Node nodes[3];
    nodes[0].value = 7;
    nodes[1].value = 8;
    nodes[2].value = 9;
    nodes[0].next = &nodes[1];
    nodes[1].next = &nodes[2];
    nodes[2].next = 0;

    int sum = 0;
    struct Node *p = &nodes[0];
    while (p != 0) {
        sum = sum + p->value;
        p = p->next;
    }

    if (sum != 24) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
