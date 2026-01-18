# Improvement Plan

## Milestones

### M1 — Minimal ABI + Syscall Spec (docs + tests)
- Define calling convention (arg/ret regs, caller/callee-saved, stack alignment).
- Document syscall numbers and semantics (read/write/exit, errors).
- Update docs and add at least 2 ABI-focused tests (call/ret, stack usage).

**Status:** Done (ABI/syscall spec updated; tests: `abi-test`, `abi-stack-test`).

### M2 — ELF Layout Control in Assembler
- Add options for text/data base addresses and segment alignment.
- Support simple linker-script-like directives (e.g., `.text`, `.data`, `.bss`, `.section`).
- Emit ELF with separate segments and correct entry point.
- Add tests that verify address placement and entry behavior.

**Status:** Done (layout options, section directives, multi-segment ELF, `elf-layout-test`).

### M3 — Expanded ISA/Test Coverage
- Add negative tests for illegal insns, misaligned accesses, cap violations.
- Add CSR edge tests (read-only, masking, delegated traps).
- Add tensor tests for boundary formats and stride edge cases.

### M4 — Quality & Tooling
- Improve assembler diagnostics (line/col, context, suggestions).
- Add CI target to run `make check`.
- Add README sections for ABI/syscall and test matrix.

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
