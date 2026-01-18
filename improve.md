# Improvement Plan

## Milestones

## Highest Priority (Immediate)

- Implement all instructions defined in the ISA spec in the simulator.
- Add tests to validate each instruction’s behavior in the simulator.

### M4 — Quality & Tooling
- Improve assembler diagnostics (line/col, context, suggestions).
- Add CI target to run `make check`.
- Add README sections for ABI/syscall and test matrix.

### M5 — Tensor Edge-Case Tests
- Add tests for NaN/INF behavior.
- Add tests for saturation behavior.
- Add tests for illegal format combinations.

**Status:** Done (tests: `tensor-naninf-test`, `tensor-sat-test`, `tensor-illegal-fmt-test`).

### M6 — System State-Transition Tests
- Add explicit `sret`/`mret` state bit tests (MIE/SIE stacking).

**Status:** Done (test: `system-ret-bits-test`).

### M7 — Interrupt Modeling Coverage
- Add basic interrupt injection coverage (`mie`/`mip` paths).

**Status:** Done (test: `interrupt-basic-test`).

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
