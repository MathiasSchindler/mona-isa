# Simulator Capabilities & Limitations

## Scope
This document describes the software simulator for the MINA architecture: what it implements, what it omits, and known behavioral constraints.

## Capabilities
- Executes MINA base integer ISA (implemented subset) with arithmetic, logic, branches, jumps, loads/stores, and system ops.
- Implements M-extension ops: `mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu`.
- Implements CSR subset for M/S-mode control and trap handling.
- Implements CSR counters: `cycle`, `time`, `instret` (read-only).
- Implements capability checks (tagged capabilities, bounds, permissions) and CAP instructions.
- Implements tensor instructions and supported tensor formats.
- Implements AMO `amoswap.w` and `amoswap.d`.
- UART MMIO:
  - TX: 0x10000000 (write bytes to stdout)
  - RX: 0x10000004 (read byte from stdin)
  - STATUS: 0x10000008 (RX ready)
- Minimal syscall ABI:
  - SYS_write(1), SYS_read(2), SYS_exit(3)
- ELF64 loader (PT_LOAD) with entry point support.
- Deterministic, single-threaded execution with optional trace and register dump.

## Limitations
- Not cycle-accurate; no timing model or pipeline behavior.
- Incomplete ISA coverage; unsupported instructions trap as unimplemented.
- Shift-immediate encodings with non-zero `imm[11:6]` trap as illegal (per ISA).
- AMO coverage is limited to `amoswap.w`/`amoswap.d` (no other atomic ops yet).
- No external interrupts or timers beyond basic CSR/trap plumbing.
- No MMU/virtual memory; all addressing is physical within simulator memory.
- Memory-mapped I/O is limited to UART addresses above.
- Syscall ABI is intentionally minimal; no file system, no process model.
- Capability model is enforced for data/code access but is not a full CHERI implementation.
- ELF support is minimal (no relocations or dynamic linking).

## Known Constraints
- Program stack pointer is initialized to the top of simulator RAM (16-byte aligned).
- Maximum instruction steps are limited by the simulator CLI option.
- RX uses non-blocking reads; programs should poll STATUS when needed.

## Testing
See simulator tests in simulator/tests and the top-level `make check` for validation.
