# mina-as Optimization Roadmap

This document proposes milestones for introducing optional optimization in `mina-as`, and a structured refactor of the assembler into clearer modules.

## Goals

- Keep behavior identical by default (optimizations are opt-in).
- Enable safe, well-tested peephole optimizations for size/perf.
- Improve maintainability by splitting the monolithic source into focused modules.

## Milestones

### A1 — Module Split (Function-Line Refactor)
- Extract lexer/tokenizer into `lexer.c`/`lexer.h`.
- Extract parser/AST (or instruction IR) into `parser.c`/`parser.h`.
- Extract encoder/emitter into `emit.c`/`emit.h`.
- Extract symbol table, relocation, and section handling into `symbols.c`/`symbols.h`.
- Keep a thin `main.c` that orchestrates the pipeline.
- Add unit tests for tokenization and parsing edge cases.

**Status:** Done (split into buffer/lexer/parse/encode/elf/assemble/util modules).

### A2 — Optimization Pipeline Skeleton
- Introduce a pre-emit optimization pass stage over a simple IR.
- Add a single `-O`/`--optimize` flag (on/off). Default is off.
- Ensure `-O0` (or no flag) reproduces current output byte-for-byte for existing tests.

**Status:** Done (no-op optimizer pass wired behind `-O/--optimize`).

### A3 — Basic Peephole Optimizations
- Remove redundant moves (`addi rd, rs, 0` where safe).
- Fold `movhi + addi` into `movhi` when low bits are zero (or equivalent safe cases).
- Simplify dead writes to `r0`.
- Add golden tests showing identical behavior and smaller code size.

**Status:** Done (peephole pass for redundant moves and movhi+addi with zero low bits; tests now run with and without `-O`).

### A4 — Control-Flow-Safe Cleanup
- Eliminate unreachable instruction sequences after unconditional jumps.
- Merge consecutive label targets where safe (no intervening alignment directives).
- Add tests that include labels, branches, and mixed directives.

**Status:** Partially done (unreachable instruction removal after unconditional jumps; label-merge and A4-specific tests pending).

### A5 — Scheduling-Aware (Optional)
- Provide a no-op placeholder pass for future scheduling.
- Collect statistics (instruction count, bytes) before/after for reporting.

### A6 — Documentation & Stability
- Document optimization rules in `mina-as/README.md`.
- Provide a summary of applied optimizations when `-O/--optimize` is enabled.
- Ensure diagnostics include “optimized away” context when relevant.

## Notes on Refactor Strategy

- Start with minimal interfaces: token stream → instruction IR → encoding.
- Keep error reporting location-aware across modules.
- Avoid changing output format until after the refactor stabilizes.

## Acceptance Criteria

- All existing tests pass at `-O0` and `-O1`.
- Optimizations are opt-in and deterministic.
- Refactor results in smaller, testable source units.
