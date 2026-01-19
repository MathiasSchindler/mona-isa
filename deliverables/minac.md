# MINA C Compiler (minac) — Capabilities and Limitations (Draft)

This document defines the supported C subset and known limitations of the MINA C compiler `minac`.

---

## 1. Overview

`minac` compiles a small C subset into MINA assembly and uses `mina-as` to emit ELF or `.bin` images.

---

## 2. Invocation

- `minac [--emit-ir] [--emit-asm] [--bin] [-O] [-o out.elf] <file.c>`
- When `-o` is provided, `mina-as` is invoked to produce the output.
- `--bin` emits a raw `.bin` image (requires `-o`).
- `-O` enables IR constant folding and dead-code elimination.

---

## 3. Supported Language Subset

### 3.1 Types

- `int`
- `char`
- `void` (function return type)
- `int *` (pointers)
- Global `int` arrays with constant size and optional constant initializers
- `char *` for string literals and pointer operations

### 3.2 Expressions

- Integer literals and identifiers
- Binary ops: `+ - * /`, comparisons `== != < <= > >=`
- Logical ops: `&& || !` (short-circuit lowering)
- Unary ops: `+ - & *`
- Indexing: `a[i]` (scaled by element size)
- Function calls (up to 8 arguments)

### 3.3 Statements

- `return`
- Local declarations with optional initializer (`int x = ...;`)
- Assignment to variables, dereferences, and indices
- `if` / `else`
- `while`
- `for`
- `switch` / `case` / `default`
- `break` / `continue`
- Blocks `{ ... }`

---

## 4. Runtime and ABI Assumptions

- MINA ABI per [deliverables/abi.md](deliverables/abi.md)
- Syscalls: `write`, `read`, `exit` (simulator ABI)
- Built-ins: `puts`, `putchar`, `exit` (mapped to syscalls)

---

## 5. Optimization

- `-O` performs IR-level constant folding and dead-code elimination in straight-line code.
- No global or interprocedural optimizations.

---

## 6. Known Limitations

- No `struct`/`union`/`enum`/`typedef`.
- No arrays of locals (only global `int` arrays).
- No pointer arithmetic beyond indexing.
- No floating-point support.
- No preprocessor (`#include`, `#define`).
- No variadic functions.
- No full type checking; minimal validation only.

---

## 7. Toolchain Integration

- Uses `mina-as` for assembly and linking.
- Emits `.data` and `.text` sections; string literals live in `.data`.

