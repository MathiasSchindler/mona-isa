#include "asm.h"
#include <stdlib.h>
#include <string.h>

void buf_init(Buffer *b) {
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
}

void buf_reserve(Buffer *b, size_t n) {
    if (b->size + n <= b->cap) return;
    size_t newcap = b->cap ? b->cap * 2 : 4096;
    while (newcap < b->size + n) newcap *= 2;
    b->data = (uint8_t *)realloc(b->data, newcap);
    b->cap = newcap;
}

void buf_write(Buffer *b, const void *p, size_t n) {
    buf_reserve(b, n);
    memcpy(b->data + b->size, p, n);
    b->size += n;
}

void buf_write_u8(Buffer *b, uint8_t v) { buf_write(b, &v, 1); }
void buf_write_u16(Buffer *b, uint16_t v) { buf_write(b, &v, 2); }
void buf_write_u32(Buffer *b, uint32_t v) { buf_write(b, &v, 4); }
void buf_write_u64(Buffer *b, uint64_t v) { buf_write(b, &v, 8); }
