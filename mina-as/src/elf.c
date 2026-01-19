#include "asm.h"
#include <string.h>

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_EXEC 2
#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4

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

int write_elf_file_sections(const char *out_path, const Section *text, const Section *data, const Section *bss, uint64_t entry, uint64_t seg_align) {
    FILE *out = fopen(out_path, "wb");
    if (!out) return 0;

    Elf64_Phdr phdrs[3];
    uint16_t phnum = 0;

    if (text->buf.size > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_X;
        ph.p_vaddr = text->base;
        ph.p_paddr = text->base;
        ph.p_filesz = text->buf.size;
        ph.p_memsz = text->buf.size;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    if (data->buf.size > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_W;
        ph.p_vaddr = data->base;
        ph.p_paddr = data->base;
        ph.p_filesz = data->buf.size;
        ph.p_memsz = data->buf.size;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    if (bss->pc > 0) {
        Elf64_Phdr ph = {0};
        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_W;
        ph.p_vaddr = bss->base;
        ph.p_paddr = bss->base;
        ph.p_filesz = 0;
        ph.p_memsz = bss->pc;
        ph.p_align = seg_align;
        phdrs[phnum++] = ph;
    }

    Elf64_Ehdr eh = {0};
    eh.e_ident[0] = ELF_MAGIC0;
    eh.e_ident[1] = ELF_MAGIC1;
    eh.e_ident[2] = ELF_MAGIC2;
    eh.e_ident[3] = ELF_MAGIC3;
    eh.e_ident[4] = ELFCLASS64;
    eh.e_ident[5] = ELFDATA2LSB;
    eh.e_ident[6] = EV_CURRENT;
    eh.e_type = ET_EXEC;
    eh.e_machine = 0;
    eh.e_version = EV_CURRENT;
    eh.e_entry = entry;
    eh.e_phoff = sizeof(Elf64_Ehdr);
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum = phnum;

    uint64_t file_off = sizeof(Elf64_Ehdr) + (uint64_t)phnum * sizeof(Elf64_Phdr);
    uint64_t cur = align_up(file_off, seg_align);

    for (uint16_t i = 0; i < phnum; i++) {
        if (phdrs[i].p_filesz == 0) {
            phdrs[i].p_offset = 0;
            continue;
        }
        phdrs[i].p_offset = cur;
        cur = align_up(cur + phdrs[i].p_filesz, seg_align);
    }

    if (fwrite(&eh, 1, sizeof(eh), out) != sizeof(eh)) { fclose(out); return 0; }
    if (phnum > 0 && fwrite(phdrs, sizeof(Elf64_Phdr), phnum, out) != phnum) { fclose(out); return 0; }

    long pos = ftell(out);
    if (pos < 0) { fclose(out); return 0; }
    if ((uint64_t)pos > file_off) { fclose(out); return 0; }

    uint64_t pad = align_up((uint64_t)pos, seg_align) - (uint64_t)pos;
    for (uint64_t i = 0; i < pad; i++) fputc(0, out);

    for (uint16_t i = 0; i < phnum; i++) {
        if (phdrs[i].p_filesz == 0) continue;
        const uint8_t *buf = NULL;
        size_t size = 0;
        if (phdrs[i].p_vaddr == text->base) { buf = text->buf.data; size = text->buf.size; }
        else if (phdrs[i].p_vaddr == data->base) { buf = data->buf.data; size = data->buf.size; }
        if (!buf || size != phdrs[i].p_filesz) { fclose(out); return 0; }

        long curpos = ftell(out);
        if (curpos < 0) { fclose(out); return 0; }
        if ((uint64_t)curpos < phdrs[i].p_offset) {
            uint64_t pad2 = phdrs[i].p_offset - (uint64_t)curpos;
            for (uint64_t p = 0; p < pad2; p++) fputc(0, out);
        }
        if (size > 0 && fwrite(buf, 1, size, out) != size) { fclose(out); return 0; }
    }

    fclose(out);
    return 1;
}
