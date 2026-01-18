# MINA Capability Model (Draft)

This document defines the capability-based memory model required by the MINA architecture.

---

## 1. Overview

Capabilities are the default pointer representation. All memory accesses are validated against a capability in capability mode. Capabilities are **not** stored in the 64-bit integer register file; they live in a dedicated capability register file.

---

## 2. Capability Representation

Capabilities are 128-bit values with the following logical fields:

- **Base (64 bits):** start address
- **Length (32 bits):** byte length of range
- **Permissions (16 bits):** read/write/execute/capability
- **Tag (1 bit):** validity tag
- **Reserved (15 bits):** future use

The **tag bit** is architectural and is tracked by hardware. Capabilities spilled to memory retain tags in a tag sidecar.

### 2.1 Tag Storage

- Tags are stored in a **shadow tag memory** at 1 tag bit per 128-bit capability word.
- Tag memory is architecturally invisible to software, but must be preserved across DMA and memory scrubbing.
- **Reference implementations** may store tags in a software-managed side table (byte-per-capability) to simplify hardware; this is allowed as long as architectural behavior matches the model.

### 2.2 Sealing Semantics

Sealing marks a capability as opaque and non-dereferenceable. A sealed capability:

- Cannot be used for loads, stores, or instruction fetch.
- Cannot be modified by `csetbounds`/`csetperm`.
- May only be unsealed by `cunseal` with the matching seal type.

Any attempt to use a sealed capability in a data or instruction access traps with a capability fault.

---

## 3. Capability Register File

MINA provides 32 capability registers: `c0`–`c31`.

- Each `cN` is 128 bits.
- Capability registers are separate from integer registers.
- `c0` is defined as the **default data capability** (DDC).
- `c1`–`c30` are general-purpose capability registers.
- `c31` is the **program counter capability** (PCC) and governs instruction fetch.

---

## 4. Addressing Rules

All scalar loads/stores and atomic memory operations use **DDC** as the base capability in capability mode:

1. Effective address is computed as `EA = rs1 + imm` (using integer registers).
2. Access is authorized by DDC: `base <= EA < base + length` and required permissions.
3. If any rule fails, the access traps with a capability fault.

Instruction fetch is authorized by **PCC**. In capability mode, the fetch address must be within PCC bounds and have execute permission; otherwise the access traps with a capability fault.

Dedicated capability load/store instructions allow explicit capability movement (see Section 6). `cld`/`cst` use DDC to authorize the memory access but read/write capability values from/to any `cN`.

---

## 5. Root Capability and Boot

- On reset, M-mode sets `c0` to an **almighty** capability covering physical memory.
- The kernel must derive user-space capabilities from `c0` and install them before entering U-mode.

### 5.1 Mode Transitions and Capability Hygiene

- Capability registers are **not** automatically cleared on privilege transitions.
- Before entering a less-privileged mode, the kernel must explicitly install a safe PCC and DDC and clear or overwrite any sensitive capabilities in `c1`–`c30`.

---

## 6. Capability Instructions (Base Set)

All capability instructions use the `CAP` opcode space (see ISA encoding table in Section 7).

- `csetbounds cd, cs1, len` — constrain capability length
- `csetperm cd, cs1, perm` — set permissions
- `cseal cd, cs1, type` — seal a capability
- `cunseal cd, cs1, type` — unseal a capability
- `cgettag rd, cs1` — read tag bit (0/1)
- `cgetbase rd, cs1` — read base
- `cgetlen rd, cs1` — read length
- `cld cd, imm(rs1)` — load capability from memory (uses DDC for address validation)
- `cst cs2, imm(rs1)` — store capability to memory (uses DDC for address validation)

---

## 7. Encoding (Formal)

Capability registers are encoded using the standard `rd/rs1/rs2` fields, interpreted as `c0`–`c31` when the `CAP` opcode is used.

**Opcode Map (Capability Extension):**

| Group | Opcode (bin) | Opcode (hex) | Format | Notes |
|---|---|---|---|---|
| CAP | 0101011 | 0x2B | R/I | Capability operations |

**R-type (opcode 0x2B)**

| Instruction | funct7 | funct3 | rd | rs1 | rs2 | Notes |
|---|---|---|---|---|---|---|
| csetbounds | 0000000 | 000 | cd | cs1 | rs2 | rs2 holds length |
| csetperm | 0000000 | 001 | cd | cs1 | rs2 | rs2 holds perm mask |
| cseal | 0000000 | 010 | cd | cs1 | rs2 | rs2 holds seal type |
| cunseal | 0000000 | 011 | cd | cs1 | rs2 | rs2 holds seal type |
| cgettag | 0000000 | 100 | rd | cs1 | r0 | rd receives 0/1 |
| cgetbase | 0000000 | 101 | rd | cs1 | r0 | rd receives base |
| cgetlen | 0000000 | 110 | rd | cs1 | r0 | rd receives length |

Notes:
- For `cset*`, `cseal`, `cunseal`, and `cld`/`cst`, `rd/rs1/rs2` denote capability registers (`c0`–`c31`).
- For `cget*`, `rd` denotes an integer register and `rs1` denotes a capability register.

**I-type (opcode 0x2B)**

| Instruction | funct3 | rd | rs1 | imm[11:0] | Notes |
|---|---|---|---|---|---|
| cld | 000 | cd | rs1 | imm | Load capability |
| cst | 001 | cs2 | rs1 | imm | Store capability |

---

## 8. Modes

- **Capability mode (default):** DDC is enforced for all scalar loads/stores.
- **Legacy mode (bootstrap only):** Capability checks are bypassed. Controlled by `mstatus.CAP`.

---

## 9. Exceptions

Capability faults are precise and synchronous. Fault subcodes are defined in [deliverables/traps.md](deliverables/traps.md).

---

## 10. Tensor Interaction

Tensor memory operations (`tld`/`tst`) are subject to the same DDC capability checks as scalar loads/stores.
