# MINA Simulator (C)

Minimal, dependency-free C simulator for the MINA ISA. Designed for step-by-step inspection and tracing while software and tools evolve.

## Build

```sh
make
```

## Run

```sh
./mina-sim program.bin
```

Options:

- `-t` enable instruction trace (PC + opcode + key fields)
- `-r` dump registers after each step
- `-s N` max steps before halt (default: 1,000,000)
- `-m N` memory size in bytes (default: 64 MiB)
- `-e HEX` entry PC (default: 0)

## Notes

- Implements a substantial base ISA subset: integer ALU, shifts, loads/stores, branches, jumps, movhi/movpc, fence, CSRs, trap entry.
- Capability ops (`CAP` opcode) and tensor ops (`TENSOR` opcode) are implemented.
- Tensor formats supported: FP32, FP16, BF16, FP8 (E4M3/E5M2), INT8, FP4 (E2M1).
- UART MMIO: store to $0x10000000$ prints bytes to stdout; load from $0x10000004$ reads a byte from stdin; load from $0x10000008$ returns 1 if data is available.
- Misaligned instruction fetch or data access traps.
- Loads ELF64 binaries (little-endian) and raw binaries.

## Syscall ABI (minimal)

System calls use `ecall` with arguments in registers:

- `r10` = a0, `r11` = a1, `r12` = a2, `r17` = a7

Implemented calls:

- `a7 = 1` (`SYS_write`): `a0=fd (1 stdout, 2 stderr)`, `a1=buf`, `a2=len` → returns bytes written in `a0`.
- `a7 = 2` (`SYS_read`): `a0=fd (0 stdin)`, `a1=buf`, `a2=len` → returns bytes read in `a0`.
- `a7 = 3` (`SYS_exit`): exits the simulator (treated like `ebreak`).

Notes:

- `len` is capped at 4096 bytes.
- Invalid file descriptors return `0` (no negative errno in v1).

## Next Steps (Suggested)

- Add a minimal syscall/ABI for console I/O
- Add ELF section/segment layout options in the assembler
- Add more self-checking tests (CSR, traps, CAP) from assembler output
