# Assembler/Linker Capabilities & Limitations

## Scope
This document describes the MINA assembler (mina-as) and current linker/ELF handling.

## Assembler Capabilities
- Two-pass assembler with labels and forward references.
- Directives: .org, .align, .byte, .half, .word, .dword, .ascii, .asciz.
- Pseudo-instructions: nop, mov, not, ret, j, jr, li.
- Supports CSR, CAP, and TENSOR instruction encodings.
- Emits ELF64 by default; optional raw binary output.
- Tokenizer supports quoted strings with spaces.

## ELF/Linking Capabilities
- ELF64 writer produces a single PT_LOAD segment.
- Entry point is derived from the first instruction address (via .org) or default 0.
- Suitable for the simulator’s minimal ELF loader.

## Limitations
- No full linker: no relocation processing, no symbol resolution across objects.
- No separate object files or archives; input is a single .s file.
- No section layout control beyond .org/.align.
- Minimal ELF metadata (no section headers, no debug info).
- Instruction coverage depends on ISA subset implemented in the assembler.

## Intended Use
- Small, self-contained assembly programs targeting the simulator.
- Tests and examples under mina-as/tests.
