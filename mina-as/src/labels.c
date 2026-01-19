#include "asm.h"
#include <string.h>

void label_add(LabelTable *t, const char *name, uint64_t addr) {
    for (size_t i = 0; i < t->count; i++) {
        if (strcmp(t->labels[i].name, name) == 0) {
            t->labels[i].addr = addr;
            return;
        }
    }
    if (t->count < MAX_LABELS) {
        strncpy(t->labels[t->count].name, name, sizeof(t->labels[t->count].name) - 1);
        t->labels[t->count].addr = addr;
        t->count++;
    }
}

int label_find(const LabelTable *t, const char *name, uint64_t *out) {
    for (size_t i = 0; i < t->count; i++) {
        if (strcmp(t->labels[i].name, name) == 0) {
            *out = t->labels[i].addr;
            return 1;
        }
    }
    return 0;
}
