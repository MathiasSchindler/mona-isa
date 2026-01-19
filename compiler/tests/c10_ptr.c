int main() {
    int x = 41;
    int *p = &x;
    *p = *p + 1;
    return x;
}
