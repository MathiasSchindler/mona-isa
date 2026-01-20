#define A 1
#ifdef A
int g = 10;
#endif

#undef A
#ifndef A
int h = 20;
#endif

#ifdef B
#define X 5
#endif

#ifndef X
int x = 7;
#endif

int main() {
    if (g != 10) { putchar(70); return 1; }
    if (h != 20) { putchar(70); return 1; }
    if (x != 7) { putchar(70); return 1; }
    putchar(79);
    putchar(75);
    return 0;
}
