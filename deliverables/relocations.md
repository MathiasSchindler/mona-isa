# MINA Relocations and Constant Synthesis (Draft)

This document defines relocation behavior for `movhi`/`addi` and related constant materialization.

**Status:** Relocations are specified here but are **not implemented** in the current toolchain. The assembler emits a single ELF image without relocation records, and there is no linker or relocation processing yet. Treat these rules as a forward-looking spec.

---

## 1. Relocation Sequences

### 1.1 32-bit Absolute

1. `movhi rd, %hi(symbol)`
2. `addi rd, rd, %lo(symbol)`

Definitions:

- `%hi(symbol)` is the upper 20 bits of the symbol address, adjusted for carry from the low 12 bits.
- `%lo(symbol)` is the low 12 bits, sign-extended.

### 1.2 64-bit Absolute

64-bit absolute addresses are materialized with a two-register sequence (canonical form):

1. `movhi rd, %hi32(symbol)`
2. `addi rd, rd, %lo32(symbol)`
3. `slli rd, rd, 32`
4. `movhi rt, %hi32lo(symbol)`
5. `addi rt, rt, %lo32lo(symbol)`
6. `or rd, rd, rt`

Definitions:

- `%hi32(symbol)` / `%lo32(symbol)` form the upper 32 bits of the address using the standard `movhi` + `addi` pair.
- `%hi32lo(symbol)` / `%lo32lo(symbol)` form the lower 32 bits of the address using the same pairing.
- Carry adjustment rules match the 32-bit absolute case for each half.

### 1.3 PC-Relative

PC-relative addressing uses `movpc` + `addi`:

1. `movpc rd, %pchi(symbol)`
2. `addi rd, rd, %pclo(symbol)`

This enables position-independent code for local references.

---

## 2. Relocation Types

| Relocation | Applies To | Description |
|---|---|---|
| R_MINA_MOVHI | `movhi` | High 20 bits (with carry adjustment) |
| R_MINA_ADDI_LO | `addi` | Low 12 bits (sign-extended) |
| R_MINA_MOVHIGHEST | `movhi` | Reserved (legacy 64-bit sequence) |
| R_MINA_ADDI_HIGHER | `addi` | Reserved (legacy 64-bit sequence) |
| R_MINA_MOVHI32 | `movhi` | Upper 32-bit high 20 bits |
| R_MINA_ADDI32_LO | `addi` | Upper 32-bit low 12 bits |
| R_MINA_MOVHI32LO | `movhi` | Lower 32-bit high 20 bits |
| R_MINA_ADDI32LO_LO | `addi` | Lower 32-bit low 12 bits |
| R_MINA_MOVPC | `movpc` | PC-relative high 20 bits |
| R_MINA_ADDI_PCLO | `addi` | PC-relative low 12 bits |
| R_MINA_JAL | `jal` | 21-bit PC-relative jump (4-byte aligned target) |
| R_MINA_BRANCH | `beq/bne/...` | 13-bit PC-relative branch (4-byte aligned target) |

---

## 3. PC-Relative Rules

- `jal` and branch offsets are relative to the address of the instruction.
- The assembler does **not** currently emit relocation records; unresolved symbol handling is out of scope for now.

---

## 4. Dynamic Linking (v1 Scope)

v1 does **not** define GOT/PLT relocations. Position-independent executables are supported via `movpc` + `addi`, but shared-library loading is out of scope.

---

## 5. Assembler Guarantees

 - `li` expands to `addi` if the immediate fits 12-bit signed.
 - Otherwise `li` expands to `movhi` + `addi` with relocation pairing.

---

## 6. Linker Constraints

The linker must ensure the `movhi`/`addi` pair remains adjacent. v1 does not define any relaxation rules; linkers must not rewrite or relax these sequences. This is not implemented yet and is documented as a target behavior.

## 7. Relaxation (Reserved)

Instruction relaxation is reserved for a future revision. Any relaxation performed by a toolchain in v1 is non-standard and must not be relied upon for correctness or compatibility.
