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

### M11 — C Data Types and Layout
- Add local arrays, pointer arithmetic, and `sizeof`.
- Add `struct`/`union` declarations and field access.
- Validate alignment and layout rules with tests.

### M12 — C Preprocessor (Minimal)
- Support `#include` and object-like `#define`.
- Add error handling for missing includes and recursive includes.
- Add tests for include paths and macro expansion.

### M13 — Assembler Enhancements
- Add `.bss` and `.zero` support.
- Add `.include` and a minimal `.macro` / `.endm`.
- Add debug info passthrough for line mapping.

### M14 — Browser Simulator (Optional, Long-Term)
- Build the simulator with Emscripten for a web demo.
- Provide a minimal UI to upload/run `.elf` or `.bin` and view stdout.
- Document constraints and limitations vs. native execution.

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

## Deliverables Gaps (from docs)

- Add relocation expressions (`%hi/%lo`) and emit relocation records in the assembler.
- Implement a minimal linker or relocation pass to resolve symbol references across inputs.
- Audit ISA/CSR coverage vs. deliverables and add tests for any missing instructions or CSR behaviors.
