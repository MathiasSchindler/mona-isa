int sum_many(int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7) {
    int locals[16];
    int i = 0;
    for (i = 0; i < 16; i = i + 1) {
        locals[i] = i + a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
    }
    int sum = 0;
    for (i = 0; i < 16; i = i + 1) {
        sum = sum + locals[i];
    }
    return sum;
}

int main() {
    int v = sum_many(1,2,3,4,5,6,7,8);
    if (v != 696) {
        putchar(70);
        return 1;
    }
    putchar(79);
    putchar(75);
    return 0;
}
