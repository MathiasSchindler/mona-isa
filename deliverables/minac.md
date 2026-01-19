# MINA C Compiler (minac) — Capabilities and Limitations (Draft)

This document defines the supported C subset and known limitations of the MINA C compiler `minac`.

---

## 1. Overview

`minac` compiles a small C subset into MINA assembly and uses `mina-as` to emit ELF or `.bin` images.

---

## 2. Invocation

- `minac [--emit-ir] [--emit-asm] [--bin] [--no-start] [-O] [-o out.elf] <file.c>`
- When `-o` is provided, `mina-as` is invoked to produce the output.
- `--bin` emits a raw `.bin` image (requires `-o`).
- `--no-start` omits the default `_start` stub for library-style assembly output.
- `-O` enables IR constant folding and dead-code elimination.

---

## 3. Supported Language Subset

### 3.1 Types

- `int`, `char`, `void` (function return type)
- Pointers to `int`/`char` and pointer-to-struct/union
- `enum` declarations (compile-time integer values)
- `struct` and `union` declarations (named, with field access)
- Global and local arrays with constant size and optional constant initializers
- String literals (`char *`) stored in `.data`

### 3.2 Expressions

- Integer and character literals, identifiers
- Binary ops: `+ - * /`, comparisons `== != < <= > >=`
- Logical ops: `&& || !` (short-circuit lowering)
- Unary ops: `+ - & *`
- Indexing: `a[i]` (scaled by element size)
- `sizeof` for types and expressions
- Function calls (up to 8 arguments)

### 3.3 Statements

- `return`
- Local declarations with optional initializer (`int x = ...;`)
- Assignment to variables, dereferences, fields, and indices
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
- Optional clib support via `#include "clib.h"` (limited include search)

---

## 5. Optimization

- `-O` performs IR-level constant folding and dead-code elimination in straight-line code.
- No global or interprocedural optimizations.

---

## 6. Known Limitations

- No `typedef`, `const`, `volatile`, or bitfields.
- No floating-point types or operations.
- No function pointers.
- No variadic functions.
- Preprocessor is minimal: only `#include "clib.h"` is supported; no `#define` or general include paths.
- Limited type checking and diagnostics compared to a full C compiler.

---

## 7. Toolchain Integration

- Uses `mina-as` for assembly and linking.
- Emits `.data` and `.text` sections; string literals live in `.data`.

