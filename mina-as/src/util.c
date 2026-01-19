#include "asm.h"
#include <string.h>

int ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    if (su > sl) return 0;
    return strcmp(s + sl - su, suffix) == 0;
}

uint64_t align_up(uint64_t v, uint64_t a) {
    if (a == 0) return v;
    uint64_t m = a - 1;
    return (v + m) & ~m;
}

int section_from_name(const char *s, SectionKind *out) {
    if (!s) return 0;
    const char *name = s;
    if (name[0] == '.') name++;
    if (strcmp(name, "text") == 0) { *out = SEC_TEXT; return 1; }
    if (strcmp(name, "data") == 0) { *out = SEC_DATA; return 1; }
    if (strcmp(name, "bss") == 0) { *out = SEC_BSS; return 1; }
    return 0;
}
