# MINA Assembler (mina-as) — Capabilities and Limitations (Draft)

This document defines the supported feature set and known limitations of the MINA assembler `mina-as`.

---

## 1. Overview

`mina-as` assembles MINA assembly into ELF or raw `.bin` images. It supports a core directive set, labels, and relocations required by the simulator toolchain.

---

## 2. Inputs and Outputs

- **Input:** MINA assembly source (`.s`).
- **Output:**
  - ELF executable (default)
  - Raw binary (`--bin`) with explicit base configuration

---

## 3. Supported Directives

- **Sections:** `.text`, `.data`, `.rodata`.
- **Placement:** `.org` (set location counter).
- **Data:** `.byte`, `.half`, `.word`, `.dword`, `.ascii`, `.asciz`.
- **Alignment:** `.align` (power-of-two alignment).
- **Symbols:** label definitions for code and data.
- **Ignored:** `.file`, `.loc` (accepted and ignored for toolchain compatibility).

---

## 4. Instruction Support

- Accepts the current MINA ISA as documented in [deliverables/isa.md](deliverables/isa.md).
- Supports CSR, CAP, and TENSOR instruction encodings.
- Pseudo-instructions: `nop`, `mov`, `not`, `ret`, `j`, `jr`, `li`.

---

## 5. Linking and Layout

- Produces a single executable image (ELF64 by default).
- ELF output is a single PT_LOAD segment, suitable for the simulator loader.
- For `--bin`, base addresses can be specified (used by the compiler integration).

---

## 6. Diagnostics

- Reports errors with line/column information where possible.
- Unknown directives or unsupported constructs generate hard errors.

---

## 7. Known Limitations

- No macro system or conditional assembly.
- No include directives.
- No relocation expressions or cross-file linking.
- No debug symbol emission.
- Directive set is intentionally minimal (only the directives listed above are guaranteed).

