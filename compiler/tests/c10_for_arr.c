int arr[4] = {1, 2, 3, 4};

int main() {
    int sum = 0;
    int i = 0;
    for (i = 0; i < 4; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}
