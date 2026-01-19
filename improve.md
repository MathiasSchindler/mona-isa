# Improvement Plan

## Milestones

### M4 — Quality & Tooling
- Improve assembler diagnostics (line/col, context, suggestions).
- Add CI target to run `make check`.
- Add README sections for ABI/syscall and test matrix.

### M8 — Optional Assembler Optimizations
- Add a flag to enable peephole optimizations (e.g., remove redundant moves, fold movhi+addi where safe).
- Add a flag to control optimization level (none/basic).
- Add tests to ensure optimized output preserves behavior.

### M9 — C Toolchain Path
- Document a recommended C cross-compiler flow (e.g., LLVM/Clang target triple, minimal runtime, linker script).
- Add a tiny C "hello" or arithmetic test compiled to MINA as a validation target.

### M10 — C Usability Gaps
- Add a basic register allocator with spilling to avoid “too many temporaries”.
- Implement pointer/array basics (address-of, dereference) for real data handling.
- Provide minimal libc stubs (`puts`, `putchar`, `exit`) over syscalls.
- Extend statements (`for`, `&&`, `||`, `switch`) and core types (`void`, `char`).

## Implementation Guidelines

- Prefer small, testable changes and keep behavior backward-compatible.
- Every new feature must include at least one self-checking test.
- Avoid introducing external dependencies unless necessary.
- Keep simulator deterministic; avoid timing-dependent behavior in tests.
- Document new flags and options in root README and component README.
- For assembly helpers, favor shared routines in a small “lib” file.

## Test Parameters

- **Build & integration**: `make check` at repo root.
- **Simulator tests**: `make tests` in `simulator/`.
- **Assembler build**: `make` in `mina-as/`.
- **ELF vs raw**: tests should run both `.elf` and `.bin` where relevant.
- **Exit criteria**:
  - All tests PASS with per-test output lines.
  - New tests must validate both stdout and program state (register/memory) when applicable.
  - No regressions in existing examples.

## Test Matrix (initial)

- **ABI**: function call/return, stack frame, caller/callee-saved regs.
- **Syscalls**: write/read/exit, invalid fd behavior.
- **ELF**: text/data placement, entry address.
- **Traps**: illegal instruction, misaligned load/store, ecall delegation.
- **CAP**: tag check, bounds/perm faults.
- **Tensor**: FP8/FP4 formats, stride handling.
