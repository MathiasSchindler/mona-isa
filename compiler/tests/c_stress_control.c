int main() {
    int i = 0;
    int j = 0;
    int sum = 0;
    for (i = 0; i < 6; i = i + 1) {
        for (j = 0; j < 6; j = j + 1) {
            if (i == 2) {
                if (j == 3) {
                    continue;
                }
            }
            if (i == 4) {
                if (j == 1) {
                    break;
                }
            }
            int k = i + j;
            int r = k - (k / 3) * 3;
            switch (r) {
                case 0:
                    sum = sum + 1;
                    break;
                case 1:
                    sum = sum + 2;
                    break;
                default:
                    sum = sum + 3;
                    break;
            }
        }
    }
    if (sum != 59) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
