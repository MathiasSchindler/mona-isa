# C Compiler Plan (MINA Target)

Goal: Implement a small C compiler that outputs MINA-compatible binaries runnable in the simulator. All work happens under `compiler/`.

Assumption: Use `mina-as` for assembly and linking (yes, that is the intended path here). The compiler will emit MINA assembly and then invoke `mina-as` to produce `.elf` (preferred) or `.bin` for the simulator.

## Roadmap — Incremental Improvements (Post-C13)

### Milestone N1 — Diagnostics and Error Recovery
**Scope**
- Improve parse/lower errors and keep compiling after a single error when possible.

**Implementation Steps**
1. Add context-rich error messages (token excerpt + caret).
2. Implement simple panic-mode recovery at statement boundaries.
3. Add a `--max-errors` flag.

**Tests**
- `tests/n1_err_recovery.c`: multiple errors reported in one run.

**Status:** Done (caret diagnostics, panic-mode recovery, `--max-errors`).

### Milestone N2 — Constant Expressions in Initializers
**Scope**
- Support constant expressions for globals and local initializers.

**Implementation Steps**
1. Add a const-eval pass for simple expressions.
2. Allow `int x = 2+3*4;` in declarations.

**Tests**
- `tests/n2_const_init.c`: constant expressions in globals/locals.

**Status:** Done.

**Notes**
- Global scalar and aggregate initializers are evaluated as constant expressions.
- Supported operators include `+`, `-`, `*`, `/`, unary `+`/`-`, and parentheses; `sizeof` is accepted where applicable.
- Local initializers accept full expressions at runtime, so constant-only restrictions do not apply there.

### Milestone N3 — Enum Types
**Scope**
- Add `enum` declarations and use as integer constants.

**Implementation Steps**
1. Parse enums and store values in symbol table.
2. Allow use in expressions and switch cases.

**Tests**
- `tests/n3_enum.c`: enum values in arithmetic and switch.

**Status:** Done (enums parsed as int type with constant values).

### Milestone N4 — Struct/Union Initializers
**Scope**
- Support `{...}` aggregate initializers for locals and globals.

**Implementation Steps**
1. Parse initializer lists for structs/unions/arrays.
2. Lower to stores or data sections.

**Tests**
- `tests/n4_init_struct.c`: struct initializer and field access.

**Status:** Done (aggregate initializer lists for arrays/structs/unions in locals and globals).

### Milestone N5 — Multi-file Compilation (Manual Link)
**Scope**
- Allow compiling a single `.c` into assembly/object without linking.

**Implementation Steps**
1. Add `--emit-obj` or `-c` option to skip linking.
2. Allow `mina-as` to link multiple `.s` files.

**Tests**
- `tests/n5_multi_file/`: two translation units linked together.

### Milestone N6 — Basic Debug Info
**Scope**
- Emit `.file`/`.loc` directives for better traceability.

**Implementation Steps**
1. Track line/col through lowering into IR.
2. Emit `.loc` entries in codegen.

**Tests**
- `tests/n6_debug_loc.c`: verify `.loc` lines in emitted asm.

### Milestone N7 — Improved Optimizations
**Scope**
- Add simple CSE and strength reduction.

**Flag Policy**
- Keep a single `-O` optimization flag; do not introduce multiple optimization flags unless absolutely necessary.

**Implementation Steps**
1. Identify repeated expressions in straight-line blocks.
2. Replace `x * 2` with `x + x` where legal.

**Tests**
- `tests/n7_cse.c`: repeated subexpression elimination.

**Status:** Done.

**Notes**
- Local value numbering removes duplicate loads and pure binary ops within a basic block.
- Strength reduction rewrites `x * 2` to `x + x` (and folds trivial `* 0`/`* 1`).

### Milestone N8 — Better Calling Convention Stress
**Scope**
- Validate >8 args and stack-passed arguments.

**Implementation Steps**
1. Support stack argument passing.
2. Update call lowering and codegen for stack args.

**Tests**
- `tests/n8_call_stack.c`: function with 10+ args.

### Milestone N9 — Preprocessor Enhancements
**Scope**
- Add `#ifndef/#ifdef/#endif` and `#undef`.

**Implementation Steps**
1. Implement conditional blocks.
2. Add `#undef` to macro table.

**Tests**
- `tests/n9_ifdef.c`: conditional compilation paths.

**Status:** Done.

**Notes**
- Added `#ifdef`, `#ifndef`, `#endif`, and `#undef` to the preprocessor.
- Directives inside inactive blocks are skipped except for conditional nesting.

### Milestone N10 — Compiler/Assembler Regression Suite
**Scope**
- Expand tests to catch toolchain regressions.

**Optimization Note**
- All optimization-related tests should continue to run under the single `-O` flag.

**Implementation Steps**
1. Add stress tests for stack, spills, labels, and pointers.
2. Add a “full suite” target in `Makefile`.

**Tests**
- Re-run all existing and new stress tests.

## Tooling Notes
- Use `mina-as` for assembling and linking `.elf` output.
- Keep all compiler-specific code and tests in `compiler/`.
- Prefer integration tests that run the simulator and check exit code or output.
