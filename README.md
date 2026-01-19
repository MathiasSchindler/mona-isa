# MINA

MINA is a small toolchain and simulator for a custom ISA. It includes an assembler, a simulator, a simple C compiler used for tests and demos, and a set of technical specifications. The focus is on clarity and reproducibility rather than feature breadth.

**Note:** This project is mostly written by GPT-5.2-Codex.
**License:** CC-0.

## Repository layout

- `deliverables/` Technical specifications (ISA, CSR, capabilities, traps, relocations, tensor extension). This is the single source of truth for the architecture.
- `docs/` Emscripten build artifacts for the website version.
- `plans/` Feature ideas and future work notes.
- `mina-as/` Two-pass assembler (`mina-as`) and its tests.
- `simulator/` Instruction-level simulator (`mina-sim`).
- `compiler/` Simple C compiler used for tests and demo programs.
- `clib/` Minimal C library used by the compiler tests and demos.
- `tools/` Utility scripts (for example, PDF generation).
- `out/` Build outputs from tests and demos (ELF files in `out/elf`, temporary files in `out/tmp`).

## Quick start

Build the assembler:

- `cd mina-as && make`

Build the simulator:

- `cd simulator && make`

Assemble example programs:

- `cd mina-as && make examples`

Run a test program:

- `cd mina-as && ../simulator/mina-sim tests/bin/hello.bin`

Run the full test suite from the repository root:

- `make test`

## UART MMIO

The simulator exposes a small UART interface:

- TX: store to `0x10000000` prints bytes to stdout
- RX: load from `0x10000004` reads a byte from stdin (0 if none)
- STATUS: load from `0x10000008` returns 1 if data is available

## Notes

- The simulator loads raw binaries and ELF64 (little-endian) binaries.
- The assembler emits ELF64 by default; use `--bin` for raw binary output.
- ELF layout can be controlled with `--text-base`, `--data-base`, `--bss-base`, and `--segment-align` (see mina-as/README.md).

## ABI & Syscalls

The base calling convention and syscall ABI are documented in `deliverables/abi.md`. The simulator implements minimal syscalls: `read`, `write`, and `exit`.
