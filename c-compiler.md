# C Compiler Plan (MINA Target)

Goal: Implement a small C compiler that outputs MINA-compatible binaries runnable in the simulator. All work happens under `compiler/`.

Assumption: Use `mina-as` for assembly and linking (yes, that is the intended path here). The compiler will emit MINA assembly and then invoke `mina-as` to produce `.elf` (preferred) or `.bin` for the simulator.

## Milestone C0 — Project Skeleton
**Scope**
- Create `compiler/` build layout and minimal driver.
- Add a `README.md` describing build/run.

**Implementation Steps**
1. Set up `compiler/src` and a `compiler/Makefile` to build a `minac` binary.
2. Add a `minac` CLI that reads a `.c` file and prints a stub message.
3. Add a `tests` folder and a tiny test harness script.

**Tests**
- `tests/c0_smoke.c`: compile command runs and prints a “not implemented” message.
- `tests/c0_usage.c`: invalid args show help and non-zero exit.

## Milestone C1 — Lexer and Parser (Tiny C Subset)
**Scope**
- Implement a lexer and parser for a strict subset: `int`, `return`, integer literals, identifiers, `+ - * /` with precedence, and parentheses.

**Implementation Steps**
1. Define tokens for keywords, identifiers, numbers, and operators.
2. Implement Pratt or recursive-descent parsing into an AST.
3. Add basic syntax error reporting with line/col.

**Tests**
- `tests/c1_expr.c`: `int main(){ return 2+3*4; }` parses.
- `tests/c1_paren.c`: `return (2+3)*4;` parses.
- `tests/c1_err.c`: missing `;` yields syntax error with line/col.

## Milestone C2 — AST Lowering to IR
**Scope**
- Define a simple IR (three-address or stack-based) for expressions and returns.

**Implementation Steps**
1. Add IR node types for constants, binary ops, and return.
2. Lower AST to IR with temporaries.
3. Add an IR pretty-printer for debugging.

**Tests**
- `tests/c2_ir.c`: compare IR output to golden file.
- `tests/c2_ir_err.c`: invalid AST cannot be lowered.

**Status:** Done (AST + IR lowering + `--emit-ir`).

## Milestone C3 — MINA Assembly Codegen
**Scope**
- Generate MINA assembly for the IR, using `mina-as` for assembly and linking.
- Support `main` returning an integer (exit code in a register or via syscall per existing ABI).

**Implementation Steps**
1. Define calling convention per existing MINA ABI docs.
2. Emit `.text` with `_start` or `main` entry per simulator expectations.
3. Implement codegen for constants, binary ops, and return.
4. Integrate a `minac --emit-asm` option.

**Tests**
- `tests/c3_return.c`: `return 7;` => simulator exits with 7.
- `tests/c3_add.c`: `return 1+2+3;` => exit 6.
- `tests/c3_mul.c`: `return 6*7;` => exit 42.

## Milestone C4 — Variables and Assignments
**Scope**
- Add local variables, assignment, and simple statements.

**Implementation Steps**
1. Add symbol table for locals with stack offsets.
2. Implement stack frame prologue/epilogue.
3. Emit loads/stores for locals.

**Tests**
- `tests/c4_vars.c`: `int main(){ int x=3; int y=4; return x+y; }` => 7.
- `tests/c4_reassign.c`: reassign and return value.

## Milestone C5 — Control Flow (if/else, while)
**Scope**
- Implement `if/else` and `while` for integer comparisons.

**Implementation Steps**
1. Add relational operators `== != < <= > >=` to parser.
2. Emit branch and label sequences for conditionals and loops.
3. Implement short-circuit logic for `&&` and `||` (optional sub-step).

**Tests**
- `tests/c5_if.c`: conditional return path.
- `tests/c5_while.c`: compute sum in loop.

## Milestone C6 — Function Calls (No Pointers Yet)
**Scope**
- Add function definitions and calls with integer arguments.

**Implementation Steps**
1. Implement function symbol table and call frame layout.
2. Emit call/return sequences using MINA ABI.
3. Enforce prototype checks for args.

**Tests**
- `tests/c6_call.c`: `add(2,3)` returns 5.
- `tests/c6_nested.c`: call from another function.

## Milestone C7 — Global Data and Strings
**Scope**
- Implement global variables and string literals placed in `.data`.

**Implementation Steps**
1. Emit `.data` section for globals and `.text` for code.
2. Add `char*` string literal lowering.
3. Add minimal `puts`-style syscall wrapper (if needed).

**Tests**
- `tests/c7_global.c`: global int initialized and read.
- `tests/c7_string.c`: write a string via syscall.

## Milestone C8 — Integration with `mina-as` and Simulator
**Scope**
- Produce runnable `.elf` and `.bin` artifacts and verify in simulator.

**Implementation Steps**
1. Add `minac -o out.elf` that runs `mina-as` as a subprocess.
2. Add a test harness to compile and run in simulator.
3. Ensure deterministic output and stable error handling.

**Tests**
- `tests/c8_smoke.c`: compile+run path with simulator exit code check.
- `tests/c8_bin.c`: produce `.bin` and run with simulator.

## Milestone C9 — Optimizations (Optional)
**Scope**
- Add a basic optimization flag for constant folding and dead code removal.

**Implementation Steps**
1. Implement IR-level constant folding.
2. Remove unused temporaries and unreachable blocks.
3. Add `-O` flag in `minac`.

**Tests**
- `tests/c9_fold.c`: confirm same output as unoptimized.
- `tests/c9_dead.c`: dead code removed (compare asm size or IR diff).

## Tooling Notes
- Use `mina-as` for assembling and linking `.elf` output.
- Keep all compiler-specific code and tests in `compiler/`.
- Prefer integration tests that run the simulator and check exit code or output.
