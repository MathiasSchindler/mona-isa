#ifndef MINA_MEM_H
#define MINA_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    uint8_t *ctag;
    size_t size;
    size_t ctag_size;
} Mem;

bool mem_init(Mem *m, size_t size);
void mem_free(Mem *m);

bool mem_read(Mem *m, uint64_t addr, void *out, size_t len);
bool mem_write(Mem *m, uint64_t addr, const void *in, size_t len);

bool mem_read_u8(Mem *m, uint64_t addr, uint8_t *out);
bool mem_read_u16(Mem *m, uint64_t addr, uint16_t *out);
bool mem_read_u32(Mem *m, uint64_t addr, uint32_t *out);
bool mem_read_u64(Mem *m, uint64_t addr, uint64_t *out);

bool mem_write_u8(Mem *m, uint64_t addr, uint8_t val);
bool mem_write_u16(Mem *m, uint64_t addr, uint16_t val);
bool mem_write_u32(Mem *m, uint64_t addr, uint32_t val);
bool mem_write_u64(Mem *m, uint64_t addr, uint64_t val);

bool mem_read_cap(Mem *m, uint64_t addr, void *out16, bool *tag);
bool mem_write_cap(Mem *m, uint64_t addr, const void *in16, bool tag);

#endif
