#ifndef MINA_AS_H
#define MINA_AS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_TOKENS 32
#define MAX_LABELS 1024
#define MAX_LINE 512

typedef struct {
    char name[64];
    uint64_t addr;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    size_t count;
} LabelTable;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t cap;
} Buffer;

typedef enum {
    SEC_TEXT = 0,
    SEC_DATA = 1,
    SEC_BSS = 2,
} SectionKind;

typedef struct {
    Buffer buf;
    uint64_t pc;
    uint64_t base;
} Section;

typedef struct {
    uint64_t text_base;
    uint64_t data_base;
    uint64_t bss_base;
    uint64_t seg_align;
} AsmOptions;

void buf_init(Buffer *b);
void buf_reserve(Buffer *b, size_t n);
void buf_write(Buffer *b, const void *p, size_t n);
void buf_write_u8(Buffer *b, uint8_t v);
void buf_write_u16(Buffer *b, uint16_t v);
void buf_write_u32(Buffer *b, uint32_t v);
void buf_write_u64(Buffer *b, uint64_t v);

int ends_with(const char *s, const char *suffix);
uint64_t align_up(uint64_t v, uint64_t a);
int section_from_name(const char *s, SectionKind *out);

int write_elf_file_sections(const char *out_path, const Section *text, const Section *data, const Section *bss, uint64_t entry, uint64_t seg_align);

void label_add(LabelTable *t, const char *name, uint64_t addr);
int label_find(const LabelTable *t, const char *name, uint64_t *out);

void strip_comment(char *line);
int is_label_def(const char *s);
int tokenize(char *line, char *tokens[], int max_tokens);

int parse_int(const char *s, int64_t *out);
int parse_reg(const char *s);
int parse_creg(const char *s);
int parse_treg(const char *s);

uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode);
uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode);
uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode);
uint32_t encode_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode);
uint32_t encode_u(int32_t imm20, uint32_t rd, uint32_t opcode);
uint32_t encode_j(int32_t imm, uint32_t rd, uint32_t opcode);

int64_t resolve_imm(const char *s, const LabelTable *labels, uint64_t pc, int is_pc_rel);

int emit_directive(Section *sec, SectionKind kind, char *tokens[], int count);
int assemble_line(Section *sec, SectionKind kind, char *tokens[], int count, const LabelTable *labels);
int first_pass(FILE *f, LabelTable *labels, const AsmOptions *opt);
int assemble_file(const char *in_path, const char *out_path, bool elf_output, const AsmOptions *opt);

uint32_t csr_addr_from_name(const char *s);
uint32_t tensor_func_from_name(const char *s);
uint32_t tensor_fmt_from_name(const char *s);
uint32_t tensor_redop_from_name(const char *s);

#endif
