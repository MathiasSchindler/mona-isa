# MINA Trap Model and Privilege Levels (Draft)

This document specifies exceptions, traps, and privilege levels for MINA.

---

## 1. Privilege Levels

MINA defines three privilege levels:

- **M (Machine):** full hardware access
- **S (Supervisor):** OS/kernel services
- **U (User):** unprivileged code

The privilege model is mandatory for any system that runs userland.

---

## 1.1 Status Bits

Status bits (including **CAP** and **TS**) are defined in [deliverables/csr.md](deliverables/csr.md). This document references their semantics but does not duplicate layouts.

---

## 2. Trap Entry

On any synchronous trap or interrupt:

1. The current PC is saved into `mepc`/`sepc` (mode-specific).
2. The cause is saved into `mcause`/`scause`.
3. The trap value (faulting address or instruction) is saved into `mtval`/`stval`.
4. The processor jumps to the trap vector address in `mtvec`/`stvec`.

Delegation rule (normative):
- If the current mode is U or S and the trap cause is delegated via `medeleg`/`mideleg`, the trap targets S.
- Otherwise, the trap targets M.

Mode transitions:

- U → S on syscall or user fault when delegated; otherwise U → M.
- S → M on supervisor faults when not delegated; otherwise handled in S.

---

## 3. Exception Codes (Base)

| Code | Name | Description |
|---:|---|---|
| 0 | Instruction address misaligned | PC not 4-byte aligned |
| 1 | Instruction access fault | Fetch permission or capability fault |
| 2 | Illegal instruction | Undefined opcode or bad encoding |
| 3 | Breakpoint | `ebreak` |
| 4 | Load address misaligned | Misaligned load |
| 5 | Load access fault | Non-capability access fault (e.g., page/privilege/MPU) |
| 6 | Store/AMO address misaligned | Misaligned store |
| 7 | Store/AMO access fault | Non-capability access fault (e.g., page/privilege/MPU) |
| 8 | Environment call from U | `ecall` in user mode |
| 9 | Environment call from S | `ecall` in supervisor mode |
| 10 | Environment call from M | `ecall` in machine mode |
| 11 | Capability fault | Bounds/perm/tag/sealed violation (use this code instead of access fault when CAP=1) |

Notes:
- In capability mode, code 11 is used for capability violations on instruction fetch, loads, and stores.
- Codes 1/5/7 remain available for non-capability access faults (e.g., privilege or page faults) in systems that implement additional protection mechanisms.

---

## 4. Trap Return

- `mret` returns to `mepc` and restores privilege.
- `sret` returns to `sepc` and restores privilege.

---

## 5. System Registers (Minimal)

- `mepc`, `sepc`
- `mcause`, `scause`
- `mtval`, `stval`
- `mtvec`, `stvec`
- `mstatus`, `sstatus`
- `mie`, `mip`, `sie`, `sip`
- `medeleg`, `mideleg` (trap delegation)
- `mscratch`, `sscratch` (trap scratch)

CSR addresses, access rules, and reset values are defined in [deliverables/csr.md](deliverables/csr.md).

---

## 6. Interrupts (Stub)

Interrupt routing and masking are defined in the platform specification. The ISA guarantees vector entry via `mtvec`/`stvec`.

---

## 7. Capability Fault Subcodes

Capability faults are refined using `mtval`/`stval`:

- **0x1** bounds violation
- **0x2** permission violation
- **0x3** tag invalid
- **0x4** sealed capability misuse

Software should decode the subcode to determine fault type.
