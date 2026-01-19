int f(int n) {
    int i = 0;
    int sum = 0;
    while (i < n) {
        if (i == 2) {
            sum = sum + 5;
        } else {
            sum = sum + 1;
        }
        i = i + 1;
    }
    return sum;
}

int g(int n) {
    int i = 0;
    int sum = 0;
    for (i = 0; i < n; i = i + 1) {
        if (i < 3) {
            sum = sum + 2;
        } else {
            sum = sum + 3;
        }
    }
    return sum;
}

int main() {
    int a = f(5);
    int b = g(5);
    if (a + b != 20) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
