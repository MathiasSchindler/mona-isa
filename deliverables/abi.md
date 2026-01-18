# MINA ABI — Base Calling Convention and Syscall ABI (Draft)

This document defines the base ABI for MINA userland and toolchain interoperability.

---

## 1. Register Roles

### 1.1 Integer Registers

| Register | ABI Name | Role | Save Convention |
|---|---|---|---|
| r0 | zero | Constant 0 | — |
| r1–r7 | s0–s6 | Callee-saved | Callee saves/restores |
| r8–r14 | t4–t10 | Caller-saved | Caller saves if needed |
| r15 | tp | Thread pointer | Preserved by runtime (do not modify) |
| r16–r23 | a0–a7 | Args/return | Caller-saved |
| r24–r27 | t0–t3 | Temporaries | Caller-saved |
| r28 | fp | Frame pointer | Callee-saved when used |
| r29 | gp | Global pointer | Preserved across calls |
| r30 | sp | Stack pointer | Always valid |
| r31 | ra | Return address | Caller-saved |

Notes:
- `s0`–`s6` are aliases for `r1`–`r7` (explicitly reserved for callee-saved use).
- `t4`–`t10` are aliases for `r8`–`r14`.
- `tp` is an alias for `r15` and is reserved for TLS; it is stable across calls.

### 1.2 Return Values

- `a0` contains the primary return value.
- `a1` contains the secondary return value when needed.

---

## 2. Calling Convention

### 2.1 Argument Passing

- Up to 8 arguments passed in `a0`–`a7`.
- Additional arguments are passed on the stack, right-to-left.

**Stack argument layout (64-bit):** the caller allocates space before the call and stores
extra arguments in 8-byte slots at the call-time stack pointer:

- 9th argument at `0(sp)`
- 10th argument at `8(sp)`
- 11th argument at `16(sp)`, etc.

**Variadic functions:** the caller must spill `a0`–`a7` to a register save area in the stack frame so the callee can access unnamed arguments via the stack.

### 2.2 Stack Frame Layout

Stack grows downward and must be **16-byte aligned** at call boundaries.

Minimum frame layout (high to low addresses):

1. Outgoing argument spill area (if any)
2. Register save area for variadic arguments (if needed)
3. Saved `ra` (if non-leaf)
4. Saved `fp` (if used)
5. Callee-saved registers (`s0`–`s6`) as needed
6. Local variables / spills

### 2.3 Prologue/Epilogue

- **Prologue:** adjust `sp`, save `ra`/`fp`/`s*`, set `fp` if used.
- **Epilogue:** restore saved registers, restore `sp`, return via `ret`.

---

## 3. Syscall ABI

### 3.1 Invocation

- `ecall` triggers a syscall.
- Syscall number in `a7`.
- Arguments in `a0`–`a5` (minimal simulator uses `a0`–`a2`).
- Return value in `a0`.
- The simulator returns `0` for invalid file descriptors or zero-length calls; memory faults trap.

### 3.2 Required Syscalls (Minimal Set)

Simulator minimal set:

- `1` — `write(fd, buf, len)`
- `2` — `read(fd, buf, len)`
- `3` — `exit(code)`

**Semantics (simulator v1):**

- `write`: `fd` must be `1` (stdout) or `2` (stderr). `len` is capped to 4096. Returns bytes written in `a0` (or 0 on invalid `fd`/zero `len`).
- `read`: `fd` must be `0` (stdin). `len` is capped to 4096. Returns bytes read in `a0` (or 0 on invalid `fd`/zero `len`).
- `exit`: halts execution (treated like `ebreak`).

A minimal userland typically requires more syscalls; numbers and semantics should remain Linux-compatible where practical.

---

## 4. Aggregate Returns

For structs/unions larger than 16 bytes, the caller passes a hidden pointer in `a0` to a caller-allocated return buffer. The callee writes the result to this buffer and returns normally; `a0` is preserved as the return pointer.

---

## 5. Floating-Point ABI

MINA uses a **soft-float** ABI in v1:

- `float`/`double` arguments are passed in integer registers (`a0`–`a7`) or on the stack.
- Return values use `a0`/`a1` as required.

---

## 6. Thread-Local Storage (TLS)

TLS uses `r15` as the thread pointer (`tp`). The runtime sets `tp` to the base of the TLS block. This register is preserved by the runtime and must not be clobbered by user code.

---

## 7. ELF and Relocations (Pointer)

Relocation and ELF ABI details are specified in [deliverables/relocations.md](deliverables/relocations.md).
