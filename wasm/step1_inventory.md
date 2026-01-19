# Step 1 Inventory — Simulator Build & Entry Point

This file documents the simulator build inputs and runtime interface needed for the WASM port.

## Sources
Simulator sources live under simulator/src:
- `main.c`
- `cpu.c`, `cpu.h`
- `mem.c`, `mem.h`
- `isa.h`

## Build (native)
From simulator/:
- Makefile builds `mina-sim` with:
  - `CC ?= cc`
  - `CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic`
  - `SRC = src/main.c src/cpu.c src/mem.c`
  - Links with `-lm`

## Entry point
- `int main(int argc, char **argv)` in simulator/src/main.c.
- Expects a path to a `.elf` or `.bin` file.
- Loads ELF64 (little-endian) via program headers; falls back to raw binary on failure.

## CLI options (from simulator/src/main.c)
- `-t` trace instructions
- `-r` dump registers after each step
- `-s N` max steps (default 1,000,000)
- `-m N` memory size bytes (default 64 MiB)
- `-e HEX` entry PC (default 0)

## ELF loading notes
- Expects ELF64 with program headers (`PT_LOAD`).
- Entry point from `e_entry` unless overridden by `-e`.
- Copies segments into simulator memory; zero-fills `memsz > filesz`.

## Browser‑WASM implications
- Uses `fopen`/`fread` to load files → in WASM, must place ELF into virtual FS.
- Minimal syscall ABI handled inside simulator, I/O via stdout/stderr.
