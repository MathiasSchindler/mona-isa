int main() {
    int i = 0;
    int j = 0;
    int sum = 0;
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            if (i == 1) {
                continue;
            }
            if (j == 2) {
                break;
            }
            sum = sum + 1;
        }
    }
    return sum;
}
