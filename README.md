# MINA

Prototype toolchain and simulator for the MINA ISA. Includes a C-based assembler, a C-based simulator with CAP/TENSOR support, and specification deliverables.

**Note:** This project is mostly written by GPT-5.2-Codex.
**License:** CC-0.

## Contents

- `deliverables/` ISA, CSR, capability, traps, relocations, tensor extension specs
- `mina-as/` Two-pass assembler (`mina-as`) and tests
- `simulator/` Instruction-level simulator (`mina-sim`)
- `tools/` Utility scripts (e.g., PDF build)

## Quick start

Build the assembler:

- `cd mina-as && make`

Build the simulator:

- `cd simulator && make`

Assemble test programs:

- `cd mina-as && make examples`

Run a test program:

- `cd mina-as && ../simulator/mina-sim tests/bin/hello.bin`

## UART MMIO

The simulator exposes a simple UART interface:

- TX: store to `0x10000000` prints bytes to stdout
- RX: load from `0x10000004` reads a byte from stdin (0 if none)
- STATUS: load from `0x10000008` returns 1 if data is available

## Notes

- The simulator loads raw binaries and ELF64 (little-endian) binaries.
- The assembler emits ELF64 by default; use `--bin` for raw binary output.
- ELF layout can be controlled with `--text-base`, `--data-base`, `--bss-base`, and `--segment-align` (see mina-as/README.md).

## ABI & Syscalls

The base calling convention and syscall ABI are documented in
`deliverables/abi.md`. Minimal syscalls implemented by the simulator are
`read`, `write`, and `exit`.
