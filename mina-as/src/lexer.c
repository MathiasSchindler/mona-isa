#include "asm.h"
#include <ctype.h>
#include <string.h>

void strip_comment(char *line) {
    for (char *p = line; *p; p++) {
        if (*p == '#' || *p == ';') {
            *p = '\0';
            break;
        }
    }
}

int is_label_def(const char *s) {
    size_t n = strlen(s);
    return n > 0 && s[n - 1] == ':';
}

int tokenize(char *line, char *tokens[], int max_tokens) {
    int count = 0;
    for (char *p = line; *p;) {
        while (isspace((unsigned char)*p) || *p == ',') p++;
        if (*p == '\0') break;
        if (*p == '(' || *p == ')') { p++; continue; }
        if (count >= max_tokens) break;
        if (*p == '"') {
            tokens[count++] = p;
            p++;
            while (*p) {
                if (*p == '\\' && p[1] != '\0') { p += 2; continue; }
                if (*p == '"') { p++; break; }
                p++;
            }
            if (*p) { *p = '\0'; p++; }
            continue;
        }
        tokens[count++] = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != '(' && *p != ')') p++;
        if (*p) { *p = '\0'; p++; }
    }
    return count;
}
