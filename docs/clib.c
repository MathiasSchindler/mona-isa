#include "clib.h"

int __minac_putchar(int c);
void __minac_exit(int code);

int putchar(int c) {
    return __minac_putchar(c);
}

int puts(char *s) {
    int n = 0;
    if (!s) return 0;
    while (*s) {
        putchar(*s);
        n = n + 1;
        s = s + 1;
    }
    return n;
}

void exit(int code) {
    __minac_exit(code);
}

int strlen(char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n] != 0) {
        n = n + 1;
    }
    return n;
}

char *memcpy(char *dst, char *src, int n) {
    int i = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return dst;
}

char *memset(char *dst, int v, int n) {
    char b = v;
    int i = 0;
    while (i < n) {
        dst[i] = b;
        i = i + 1;
    }
    return dst;
}

int print_int(int v) {
    char buf[32];
    int n = 0;
    int written = 0;
    if (v == 0) {
        putchar(48);
        return 1;
    }
    if (v < 0) {
        putchar(45);
        written = written + 1;
        v = 0 - v;
    }
    while (v > 0) {
        int q = v / 10;
        int digit = v - (q * 10);
        buf[n] = digit + 48;
        n = n + 1;
        v = q;
    }
    while (n > 0) {
        n = n - 1;
        putchar(buf[n]);
        written = written + 1;
    }
    return written;
}

int vprintf(char *fmt, int *args, int count) {
    int i = 0;
    int ai = 0;
    int written = 0;
    int limit = 1024;
    if (!fmt) return 0;
    while (fmt[i] != 0 && limit > 0) {
        char c = fmt[i];
        if (c == 37) {
            i = i + 1;
            char f = fmt[i];
            if (f == 0) break;
            if (f == 37) {
                putchar(37);
                written = written + 1;
            } else if (ai < count) {
                int arg = args[ai];
                if (f == 115) {
                    char *s = arg;
                    if (!s) s = "";
                    while (*s) {
                        putchar(*s);
                        written = written + 1;
                        s = s + 1;
                    }
                } else if (f == 99) {
                    putchar(arg);
                    written = written + 1;
                } else if (f == 100) {
                    written = written + print_int(arg);
                } else {
                    putchar(37);
                    putchar(f);
                    written = written + 2;
                }
                ai = ai + 1;
            }
        } else {
            putchar(c);
            written = written + 1;
        }
        i = i + 1;
        limit = limit - 1;
    }
    return written;
}

int printf(char *fmt, int a, int b, int c) {
    int args[3];
    args[0] = a;
    args[1] = b;
    args[2] = c;
    return vprintf(fmt, args, 3);
}

int isdigit(int c) {
    if (c >= 48 && c <= 57) return 1;
    return 0;
}

int isalpha(int c) {
    if (c >= 65 && c <= 90) return 1;
    if (c >= 97 && c <= 122) return 1;
    return 0;
}

int isspace(int c) {
    if (c == 32) return 1;
    if (c == 9) return 1;
    if (c == 10) return 1;
    if (c == 13) return 1;
    return 0;
}

int strtol(char *s, int *end, int base) {
    int i = 0;
    int sign = 1;
    int value = 0;
    if (!s) {
        if (end) *end = 0;
        return 0;
    }
    if (base != 10) {
        if (end) *end = 0;
        return 0;
    }
    while (s[i] == 32 || s[i] == 9 || s[i] == 10 || s[i] == 13) {
        i = i + 1;
    }
    if (s[i] == 45) {
        sign = -1;
        i = i + 1;
    } else if (s[i] == 43) {
        i = i + 1;
    }
    while (s[i] >= 48 && s[i] <= 57) {
        value = value * 10 + (s[i] - 48);
        i = i + 1;
    }
    if (end) *end = i;
    return value * sign;
}

int atoi(char *s) {
    return strtol(s, 0, 10);
}

int write(int fd, char *buf, int len) {
    int i = 0;
    if (!buf) return 0;
    if (len <= 0) return 0;
    if (fd == 1 || fd == 2) {
        while (i < len) {
            putchar(buf[i]);
            i = i + 1;
        }
        return len;
    }
    return 0;
}

int read(int fd, char *buf, int len) {
    if (!buf) return 0;
    if (len <= 0) return 0;
    if (fd != 0) return 0;
    return 0;
}

int clib_heap[8];
int clib_heap_used = 0;

char *malloc(int n) {
    if (n <= 0) return 0;
    if (clib_heap_used != 0) return 0;
    clib_heap_used = 1;
    return clib_heap;
}

void free(char *p) {
    if (p == clib_heap) clib_heap_used = 0;
    return;
}

char *calloc(int n, int size) {
    int total = n * size;
    char *p = malloc(total);
    if (!p) return 0;
    memset(p, 0, total);
    return p;
}

char *realloc(char *p, int n) {
    if (!p) return malloc(n);
    if (n <= 0) return 0;
    return p;
}
