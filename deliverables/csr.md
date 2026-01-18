# MINA CSR Map and Privilege Control (Draft)

This document defines the Control and Status Register (CSR) address map, reset values, and privilege-control policy for MINA.

---

## 1. Scope

This document specifies:

- CSR address map (M/S/U where applicable)
- Reset values for each CSR
- Access permissions by privilege level
- Side effects on read/write
- Trap delegation and vectoring behavior
- `mret`/`sret` state transitions

It is authoritative for CSR behavior referenced by the ISA and trap model.

---

## 2. CSR Address Map (Draft)

All CSR addresses are 12-bit.

### 2.1 Machine CSRs (M)

| CSR | Address | Access | Reset | Description |
|---|---|---|---|---|
| mstatus | 0x300 | R/W | 0x0000_0000_0000_0000 | Machine status (includes CAP, TS, IE bits) |
| mie | 0x304 | R/W | 0x0000_0000_0000_0000 | Machine interrupt enable |
| medeleg | 0x302 | R/W | 0x0000_0000_0000_0000 | Trap delegation (exceptions) |
| mideleg | 0x303 | R/W | 0x0000_0000_0000_0000 | Interrupt delegation |
| mtvec | 0x305 | R/W | 0x0000_0000_0000_0000 | Machine trap vector base |
| mip | 0x344 | R/W | 0x0000_0000_0000_0000 | Machine interrupt pending |
| mscratch | 0x340 | R/W | 0x0000_0000_0000_0000 | Scratch register for trap handlers |
| mepc | 0x341 | R/W | 0x0000_0000_0000_0000 | Machine exception PC |
| mcause | 0x342 | R/W | 0x0000_0000_0000_0000 | Machine trap cause |
| mtval | 0x343 | R/W | 0x0000_0000_0000_0000 | Machine trap value |
| cycle | 0xC00 | R | 0x0000_0000_0000_0000 | Cycle counter (M-mode read-only in v1) |
| time | 0xC01 | R | 0x0000_0000_0000_0000 | Time counter (platform-defined tick, M-mode read-only in v1) |
| instret | 0xC02 | R | 0x0000_0000_0000_0000 | Instructions retired (M-mode read-only in v1) |

### 2.2 Supervisor CSRs (S)

| CSR | Address | Access | Reset | Description |
|---|---|---|---|---|
| sstatus | 0x100 | R/W | 0x0000_0000_0000_0000 | Supervisor status (subset of mstatus) |
| sie | 0x104 | R/W | 0x0000_0000_0000_0000 | Supervisor interrupt enable |
| stvec | 0x105 | R/W | 0x0000_0000_0000_0000 | Supervisor trap vector base |
| sip | 0x144 | R/W | 0x0000_0000_0000_0000 | Supervisor interrupt pending |
| sscratch | 0x140 | R/W | 0x0000_0000_0000_0000 | Supervisor scratch |
| sepc | 0x141 | R/W | 0x0000_0000_0000_0000 | Supervisor exception PC |
| scause | 0x142 | R/W | 0x0000_0000_0000_0000 | Supervisor trap cause |
| stval | 0x143 | R/W | 0x0000_0000_0000_0000 | Supervisor trap value |

### 2.3 User CSRs (U)

User-mode CSRs are not required in v1. If added in future, they must be documented here with privilege rules.

---

## 3. Status Bits and Privilege Control

### 3.1 `mstatus` Bit Layout (Draft)

| Bit | Name | Description |
|---:|---|---|
| 0 | MIE | Machine interrupt enable |
| 1 | SIE | Supervisor interrupt enable |
| 4 | MPIE | Machine previous interrupt enable |
| 5 | SPIE | Supervisor previous interrupt enable |
| 8 | CAP | Capability mode enable (1=enforced, 0=legacy) |
| 10:9 | TS | Tensor state (00=off, 01=initial, 10=clean, 11=dirty) |
| 12:11 | MPP | Previous privilege mode for `mret` |
| 13 | SPP | Previous privilege mode for `sret` |

Notes:
- `sstatus` is a read/write subset of `mstatus`. Fields not exposed in `sstatus` read as 0 and ignore writes.
- `mstatus.MPP` encoding: 00=U, 01=S, 11=M (10 is reserved).
- `mstatus.SPP` encoding: 0=U, 1=S.

### 3.2 Trap Delegation

- If a trap is delegated via `medeleg`/`mideleg`, control transfers to S-mode and `sepc`/`scause`/`stval` are written.
- Otherwise, control transfers to M-mode and `mepc`/`mcause`/`mtval` are written.

### 3.3 Vectoring (`mtvec`/`stvec`)

- Base address is aligned to 4 bytes.
- Vector mode is reserved for future use; v1 uses direct mode only.

### 3.4 `mcause`/`scause` Encoding

- Bit 63 indicates interrupt (1) vs. synchronous exception (0).
- Bits [62:0] contain the interrupt or exception code.
- Exception codes are defined in [deliverables/traps.md](deliverables/traps.md).

### 3.5 Trap Entry Interrupt Stacking

On trap entry to M-mode:

- `mstatus.MPIE` is set to the prior `mstatus.MIE` value.
- `mstatus.MIE` is cleared to 0.

On trap entry to S-mode:

- `mstatus.SPIE` is set to the prior `mstatus.SIE` value.
- `mstatus.SIE` is cleared to 0.

---

## 4. CSR Instruction Legality

CSR instructions are legal only when the current privilege level meets the CSR’s minimum privilege:

- M-level CSRs: accessible only in M-mode.
- S-level CSRs: accessible in M or S-mode.
- U-level CSRs: accessible in all modes.

Illegal CSR access traps with **Illegal instruction**.

---

## 5. Trap Return Semantics

### 5.1 `mret`

- Restores privilege from `mstatus.MPP`.
- Clears `mstatus.MPP` to U after return.
- Restores `mstatus.MIE` from `mstatus.MPIE`, then sets `mstatus.MPIE` to 1.

### 5.2 `sret`

- Restores privilege from `mstatus.SPP`.
- Clears `mstatus.SPP` to U after return.
- Restores `mstatus.SIE` from `mstatus.SPIE`, then sets `mstatus.SPIE` to 1.

---

## 6. Reserved Encodings

- CSR addresses not listed in this document are reserved and must trap on access.
- Future extensions may define additional CSRs within unallocated ranges.

