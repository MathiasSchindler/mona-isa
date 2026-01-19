int putchar(int c);
int puts(char *s);
void exit(int code);

int strlen(char *s);
char *memcpy(char *dst, char *src, int n);
char *memset(char *dst, int v, int n);

int vprintf(char *fmt, int *args, int count);
int printf(char *fmt, int a, int b, int c);

int isdigit(int c);
int isalpha(int c);
int isspace(int c);

int atoi(char *s);
int strtol(char *s, int *end, int base);
