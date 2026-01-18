#include "mem.h"
#include <stdlib.h>
#include <string.h>

bool mem_init(Mem *m, size_t size) {
    m->data = (uint8_t *)calloc(1, size);
    if (!m->data) return false;
    m->ctag_size = (size + 15) / 16;
    m->ctag = (uint8_t *)calloc(1, m->ctag_size);
    if (!m->ctag) {
        free(m->data);
        m->data = NULL;
        return false;
    }
    m->size = size;
    return true;
}

void mem_free(Mem *m) {
    free(m->data);
    free(m->ctag);
    m->data = NULL;
    m->ctag = NULL;
    m->size = 0;
    m->ctag_size = 0;
}

static bool in_bounds(Mem *m, uint64_t addr, size_t len) {
    if (addr + len < addr) return false;
    return addr + len <= m->size;
}

bool mem_read(Mem *m, uint64_t addr, void *out, size_t len) {
    if (!in_bounds(m, addr, len)) return false;
    memcpy(out, &m->data[addr], len);
    return true;
}

bool mem_write(Mem *m, uint64_t addr, const void *in, size_t len) {
    if (!in_bounds(m, addr, len)) return false;
    memcpy(&m->data[addr], in, len);
    return true;
}

bool mem_read_u8(Mem *m, uint64_t addr, uint8_t *out) {
    return mem_read(m, addr, out, sizeof(*out));
}

bool mem_read_u16(Mem *m, uint64_t addr, uint16_t *out) {
    return mem_read(m, addr, out, sizeof(*out));
}

bool mem_read_u32(Mem *m, uint64_t addr, uint32_t *out) {
    return mem_read(m, addr, out, sizeof(*out));
}

bool mem_read_u64(Mem *m, uint64_t addr, uint64_t *out) {
    return mem_read(m, addr, out, sizeof(*out));
}

bool mem_write_u8(Mem *m, uint64_t addr, uint8_t val) {
    return mem_write(m, addr, &val, sizeof(val));
}

bool mem_write_u16(Mem *m, uint64_t addr, uint16_t val) {
    return mem_write(m, addr, &val, sizeof(val));
}

bool mem_write_u32(Mem *m, uint64_t addr, uint32_t val) {
    return mem_write(m, addr, &val, sizeof(val));
}

bool mem_write_u64(Mem *m, uint64_t addr, uint64_t val) {
    return mem_write(m, addr, &val, sizeof(val));
}

bool mem_read_cap(Mem *m, uint64_t addr, void *out16, bool *tag) {
    if (addr & 0xF) return false;
    if (!in_bounds(m, addr, 16)) return false;
    memcpy(out16, &m->data[addr], 16);
    uint64_t idx = addr / 16;
    if (idx >= m->ctag_size) return false;
    *tag = (m->ctag[idx] != 0);
    return true;
}

bool mem_write_cap(Mem *m, uint64_t addr, const void *in16, bool tag) {
    if (addr & 0xF) return false;
    if (!in_bounds(m, addr, 16)) return false;
    memcpy(&m->data[addr], in16, 16);
    uint64_t idx = addr / 16;
    if (idx >= m->ctag_size) return false;
    m->ctag[idx] = tag ? 1 : 0;
    return true;
}
