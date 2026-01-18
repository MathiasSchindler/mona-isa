#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

static void usage(const char *argv0) {
    printf("Usage: %s [options] program.bin\n", argv0);
    printf("Options:\n");
    printf("  -t        trace instructions\n");
    printf("  -r        dump registers after each step\n");
    printf("  -s N      max steps (default 1000000)\n");
    printf("  -m N      memory size bytes (default 67108864)\n");
    printf("  -e HEX    entry PC (hex) (default 0)\n");
}

static bool load_binary(Mem *m, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { fclose(f); return false; }
    fseek(f, 0, SEEK_SET);
    if ((size_t)size > m->size) { fclose(f); return false; }
    size_t n = fread(m->data, 1, (size_t)size, f);
    fclose(f);
    return n == (size_t)size;
}

static bool load_elf(Mem *m, const char *path, uint64_t *entry_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    Elf64_Ehdr eh;
    if (fread(&eh, 1, sizeof(eh), f) != sizeof(eh)) { fclose(f); return false; }
    if (eh.e_ident[0] != 0x7F || eh.e_ident[1] != 'E' || eh.e_ident[2] != 'L' || eh.e_ident[3] != 'F') { fclose(f); return false; }
    if (eh.e_ident[4] != 2 || eh.e_ident[5] != 1) { fclose(f); return false; }
    if (eh.e_phentsize != sizeof(Elf64_Phdr)) { fclose(f); return false; }

    for (uint16_t i = 0; i < eh.e_phnum; i++) {
        if (fseek(f, (long)(eh.e_phoff + (uint64_t)i * sizeof(Elf64_Phdr)), SEEK_SET) != 0) { fclose(f); return false; }
        Elf64_Phdr ph;
        if (fread(&ph, 1, sizeof(ph), f) != sizeof(ph)) { fclose(f); return false; }
        if (ph.p_type != 1) continue;
        if (ph.p_vaddr + ph.p_memsz > m->size) { fclose(f); return false; }
        if (ph.p_filesz > 0) {
            uint8_t *buf = (uint8_t *)malloc((size_t)ph.p_filesz);
            if (!buf) { fclose(f); return false; }
            if (fseek(f, (long)ph.p_offset, SEEK_SET) != 0) { free(buf); fclose(f); return false; }
            if (fread(buf, 1, (size_t)ph.p_filesz, f) != ph.p_filesz) { free(buf); fclose(f); return false; }
            if (!mem_write(m, ph.p_vaddr, buf, (size_t)ph.p_filesz)) { free(buf); fclose(f); return false; }
            free(buf);
        }
        if (ph.p_memsz > ph.p_filesz) {
            uint64_t zero_start = ph.p_vaddr + ph.p_filesz;
            uint64_t zero_len = ph.p_memsz - ph.p_filesz;
            if (zero_start + zero_len > m->size) { fclose(f); return false; }
            memset(&m->data[zero_start], 0, (size_t)zero_len);
        }
    }

    *entry_out = eh.e_entry;
    fclose(f);
    return true;
}

int main(int argc, char **argv) {
    size_t mem_size = 64ull * 1024 * 1024;
    uint64_t entry = 0;
    uint64_t max_steps = 1000000;
    bool trace = false;
    bool dump_regs = false;
    bool entry_override = false;

    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "-t") == 0) {
            trace = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            dump_regs = true;
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) { usage(argv[0]); return 1; }
            max_steps = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-m") == 0) {
            if (i + 1 >= argc) { usage(argv[0]); return 1; }
            mem_size = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-e") == 0) {
            if (i + 1 >= argc) { usage(argv[0]); return 1; }
            entry = strtoull(argv[++i], NULL, 16);
            entry_override = true;
        } else {
            usage(argv[0]);
            return 1;
        }
        i++;
    }

    if (i >= argc) { usage(argv[0]); return 1; }
    const char *bin_path = argv[i];

    Mem mem;
    if (!mem_init(&mem, mem_size)) {
        fprintf(stderr, "failed to allocate memory\n");
        return 1;
    }

    uint64_t elf_entry = 0;
    bool loaded = load_elf(&mem, bin_path, &elf_entry);
    if (!loaded) {
        if (!load_binary(&mem, bin_path)) {
            fprintf(stderr, "failed to load binary: %s\n", bin_path);
            mem_free(&mem);
            return 1;
        }
    } else if (!entry_override) {
        entry = elf_entry;
    }

    Cpu cpu;
    cpu_init(&cpu, entry);
    cpu.trace = trace;
    cpu.dump_regs = dump_regs;
    cpu.regs[30] = (uint64_t)mem.size & ~0xFULL;

    Trap trap = TRAP_NONE;
    uint64_t iterations = 0;
    while (iterations < max_steps) {
        trap = cpu_step(&cpu, &mem);
        if (trap != TRAP_NONE) break;
        iterations++;
    }

    if (trap != TRAP_NONE) {
        if (trap == TRAP_EBREAK) {
            fprintf(stderr, "halted on ebreak after %llu steps at pc=0x%llx\n",
                    (unsigned long long)cpu.steps,
                    (unsigned long long)cpu.pc);
            mem_free(&mem);
            return 0;
        }
        fprintf(stderr, "trap after %llu steps at pc=0x%llx: %d\n",
                (unsigned long long)cpu.steps,
                (unsigned long long)cpu.pc,
                (int)trap);
        mem_free(&mem);
        return 2;
    }

    if (iterations >= max_steps) {
        fprintf(stderr, "halted after reaching step limit (%llu)\n",
                (unsigned long long)max_steps);
    }

    mem_free(&mem);
    return 0;
}
