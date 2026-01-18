# MINA ISA — Detailed Specification (Draft)

This document expands Section 2 of the plan into a more explicit and implementable specification. It is intended as a human-readable source of truth for instruction decoding, assembly syntax, and behavioral intent.

The base integer ISA is intentionally small but complete and is expected to be **more than 40 instructions** once load/store variants, immediate forms, and control-flow primitives are fully enumerated. This is acceptable and should be treated as the canonical base.

---

## 2.1 Register File (Expanded)

### 2.1.1 Architectural Registers

MINA defines 32 general-purpose architectural registers, each 64 bits wide. All registers are uniform at the hardware level; names are ABI conveniences only.

| Register | Name | ABI Role | Notes |
|---------:|------|----------|-------|
| r0 | zero | constant 0 | Reads return 0, writes are ignored. |
| r1–r15 | general | caller/callee | Pure GP, usage by ABI conventions. |
| r16–r23 | a0–a7 | args/return | Function arguments, return values. |
| r24–r27 | t0–t3 | temporaries | Caller-saved temps. |
| r28 | fp | frame pointer | Optional; ABI-dependent. |
| r29 | gp | global pointer | PIC base (ABI). |
| r30 | sp | stack pointer | Downward-growing stack. |
| r31 | ra | return address | Link register for calls. |

### 2.1.2 Register Access Semantics

- **Reads**: All registers read as 64-bit values. There is no implicit sign/zero extension at the register file; extension only occurs for immediates and load variants.
- **Writes**: All registers except `r0` accept writes. Writing to `r0` has no effect.
- **Aliasing**: `a0–a7`, `t0–t3`, `sp`, `fp`, `gp`, `ra` are aliases for specific `rN` and are not distinct physical storage.

### 2.1.3 ABI Conventions (Informative)

ABI register roles, calling conventions, TLS, and the syscall ABI are defined in [deliverables/abi.md](deliverables/abi.md). This ISA document treats all `r1`–`r31` registers as uniform and leaves ABI policy to that specification.

---

## 2.2 Instruction Encoding (Expanded)

### 2.2.1 Fixed Width Encoding

All instructions are 32 bits. The uniform width simplifies fetch/decode, I-cache organization, and pipeline control. No compressed format is part of the base ISA.

### 2.2.2 Instruction Formats

MINA uses six base formats, derived from classic RISC encodings. The exact bit allocation is specified in the formal encoding tables below, but the semantic structure is fixed:

#### R-type (Register–Register)

Used for arithmetic/logic operations and comparisons.

- Fields: `opcode`, `rd`, `rs1`, `rs2`, `func`
- Semantics: `rd = f(rs1, rs2)` or `rd = cmp(rs1, rs2)`

#### I-type (Immediate / Load)

Used for immediate arithmetic, logic, and loads.

- Fields: `opcode`, `rd`, `rs1`, `imm`
- Semantics: `rd = f(rs1, imm)` or `rd = MEM[rs1 + imm]`

#### S-type (Store)

Used for stores.

- Fields: `opcode`, `rs1`, `rs2`, `imm`
- Semantics: `MEM[rs1 + imm] = rs2`

#### B-type (Branch)

Used for conditional branches.

- Fields: `opcode`, `rs1`, `rs2`, `imm`
- Semantics: `if cond(rs1, rs2) PC = PC + imm`

#### U-type (Upper Immediate)

Used for constructing constants with `movhi` and PC-relative values with `movpc`.

- Fields: `opcode`, `rd`, `imm[31:12]`
- Semantics: `movhi`: `rd = imm << 12` (sign-extended from bit 31)
- Semantics: `movpc`: `rd = PC + (imm << 12)` (sign-extended from bit 31)

#### J-type (Jump)

Used for direct jumps (`jal`).

- Fields: `opcode`, `rd`, `imm`
- Semantics: `rd = PC + 4; PC = PC + imm`

### 2.2.3 Immediate Semantics

- Immediates are sign-extended by default, including logical-immediate instructions (`andi`, `ori`, `xori`). This is a deliberate simplification to avoid split semantics; toolchains should emit explicit masks when zero-extension is required.
	- Example: `andi rd, rs1, 0xFF` sign-extends to 255 and behaves as a byte mask. `andi rd, rs1, 0xFFF` sign-extends to -1 and is equivalent to a bitwise NOT mask. Use explicit masks when zero-extension is intended.
- Immediates represent byte offsets for memory and control-flow instructions.
- Loads/stores use byte offsets with natural alignment requirements.
 - For shift-immediate instructions (`slli`, `srli`, `srai`), `imm[5:0]` is the shift amount and `imm[11:6]` must be zero; otherwise the instruction traps as illegal.

### 2.2.4 Alignment and Endianness

- The architecture is **little-endian** for all memory operations.
- Misaligned accesses **trap** in the base ISA. Implementations may optionally provide a higher-level emulation layer, but architectural behavior is a trap.

Capability-based address validation is defined in [deliverables/capabilities.md](deliverables/capabilities.md).

### 2.2.5 Exception/Trap Model (Base)

The ISA exposes `ecall` and `ebreak` for system calls and debugging traps. The base trap model, privilege levels, and exception codes are defined in [deliverables/traps.md](deliverables/traps.md).

### 2.2.6 Bitfield Layouts (Formal)

All bit positions are numbered `[31:0]`, where bit 31 is the MSB.

**R-type:**

- `funct7[31:25] | rs2[24:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | opcode[6:0]`

**I-type:**

- `imm[11:0][31:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | opcode[6:0]`

**S-type:**

- `imm[11:5][31:25] | rs2[24:20] | rs1[19:15] | funct3[14:12] | imm[4:0][11:7] | opcode[6:0]`

**B-type:**

- `imm[12][31] | imm[10:5][30:25] | rs2[24:20] | rs1[19:15] | funct3[14:12] | imm[4:1][11:8] | imm[11][7] | opcode[6:0]`

**U-type:**

- `imm[31:12][31:12] | rd[11:7] | opcode[6:0]`

**J-type:**

- `imm[20][31] | imm[10:1][30:21] | imm[11][20] | imm[19:12][19:12] | rd[11:7] | opcode[6:0]`

### 2.2.7 Opcode Map (Formal)

| Group | Opcode (bin) | Opcode (hex) | Format | Notes |
|---|---|---|---|---|
| OP | 0110011 | 0x33 | R | Register-register ALU ops |
| OP-IMM | 0010011 | 0x13 | I | Immediate ALU ops |
| LOAD | 0000011 | 0x03 | I | All loads |
| STORE | 0100011 | 0x23 | S | All stores |
| AMO | 0101111 | 0x2F | R | Atomic read-modify-write |
| BRANCH | 1100011 | 0x63 | B | Conditional branches |
| JAL | 1101111 | 0x6F | J | Direct jumps |
| JALR | 1100111 | 0x67 | I | Indirect jumps |
| MOVHI | 0110111 | 0x37 | U | Upper immediate move |
| MOVPC | 0010111 | 0x17 | U | PC-relative upper immediate |
| CAP | 0101011 | 0x2B | R/I | Capability operations (see capabilities; register fields address `c0`–`c31` under this opcode) |
| SYSTEM | 1110011 | 0x73 | I | ecall/ebreak |
| FENCE | 0001111 | 0x0F | I | Memory barrier |

---

## 2.3 Base Integer ISA (Expanded, Extensive)

This section fully enumerates the base integer instructions, grouped by function, including semantics and notes about edge cases.

### 2.3.0 Formal Encoding Tables (Complete Base ISA)

All encodings below use the opcode map in Section 2.2.7. Every instruction in the base ISA appears exactly once in the tables below.

**OP (R-type, opcode 0x33)**

| Instruction | funct7 | funct3 | Notes |
|---|---|---|---|
| add | 0000000 | 000 | `rd = rs1 + rs2` |
| sub | 0100000 | 000 | `rd = rs1 - rs2` |
| mul | 0000001 | 000 | Signed multiply (low 64) |
| mulh | 0000001 | 001 | Signed high 64 |
| mulhsu | 0000001 | 010 | Signed × unsigned high 64 |
| mulhu | 0000001 | 011 | Unsigned high 64 |
| div | 0000001 | 100 | Signed division |
| divu | 0000001 | 101 | Unsigned division |
| rem | 0000001 | 110 | Signed remainder |
| remu | 0000001 | 111 | Unsigned remainder |
| sll | 0000000 | 001 | Logical left shift |
| srl | 0000000 | 101 | Logical right shift |
| sra | 0100000 | 101 | Arithmetic right shift |
| and | 0000000 | 111 | Bitwise AND |
| or | 0000000 | 110 | Bitwise OR |
| xor | 0000000 | 100 | Bitwise XOR |
| slt | 0000000 | 010 | Signed compare |
| sltu | 0000000 | 011 | Unsigned compare |

**OP-IMM (I-type, opcode 0x13)**

| Instruction | funct7 | funct3 | Notes |
|---|---|---|---|
| addi | — | 000 | `rd = rs1 + imm` |
| andi | — | 111 | `rd = rs1 & imm` |
| ori | — | 110 | `rd = rs1 | imm` |
| xori | — | 100 | `rd = rs1 ^ imm` |
| slti | — | 010 | Signed compare immediate |
| sltiu | — | 011 | Unsigned compare immediate |
| slli | 0000000 | 001 | `shamt = imm[5:0]` |
| srli | 0000000 | 101 | `shamt = imm[5:0]` |
| srai | 0100000 | 101 | `shamt = imm[5:0]` |

**LOAD (I-type, opcode 0x03)**

| Instruction | funct3 | Data size | Notes |
|---|---|---|---|
| ldb | 000 | 8 | Sign-extend |
| ldbu | 100 | 8 | Zero-extend |
| ldh | 001 | 16 | Sign-extend |
| ldhu | 101 | 16 | Zero-extend |
| ldw | 010 | 32 | Sign-extend |
| ldwu | 110 | 32 | Zero-extend |
| ld | 011 | 64 | Full width |

**STORE (S-type, opcode 0x23)**

| Instruction | funct3 | Data size | Notes |
|---|---|---|---|
| stb | 000 | 8 | Store low 8 bits |
| sth | 001 | 16 | Store low 16 bits |
| stw | 010 | 32 | Store low 32 bits |
| st | 011 | 64 | Store full 64 bits |

**AMO (R-type, opcode 0x2F)**

| Instruction | funct7 | funct3 | Notes |
|---|---|---|---|
| amoswap.w | 0000100 | 010 | Atomic swap 32-bit (sign-extend) |
| amoswap.d | 0000100 | 011 | Atomic swap 64-bit |

**BRANCH (B-type, opcode 0x63)**

| Instruction | funct3 | Condition |
|---|---|---|
| beq | 000 | `rs1 == rs2` |
| bne | 001 | `rs1 != rs2` |
| blt | 100 | Signed less-than |
| bge | 101 | Signed greater-or-equal |
| bltu | 110 | Unsigned less-than |
| bgeu | 111 | Unsigned greater-or-equal |

**JAL / JALR / MOVHI / MOVPC**

| Instruction | Opcode | Format | funct3 | Notes |
|---|---|---|---|---|
| jal | 0x6F | J | — | `rd = PC + 4; PC = PC + imm` |
| jalr | 0x67 | I | 000 | `rd = PC + 4; PC = (rs1 + imm) & ~3` |
| movhi | 0x37 | U | — | `rd = sign_extend(imm20) << 12` |
| movpc | 0x17 | U | — | `rd = PC + (sign_extend(imm20) << 12)` |

**SYSTEM / FENCE / CSR**

| Instruction | Opcode | Format | funct3 | imm[11:0] | Notes |
|---|---|---|---|---|---|
| ecall | 0x73 | I | 000 | 0x000 | `rs1 = rd = 0` |
| ebreak | 0x73 | I | 000 | 0x001 | `rs1 = rd = 0` |
| mret | 0x73 | I | 000 | 0x302 | Machine return |
| sret | 0x73 | I | 000 | 0x102 | Supervisor return |
| csrrw | 0x73 | I | 001 | csr | `rd = CSR; CSR = rs1` |
| csrrs | 0x73 | I | 010 | csr | `rd = CSR; CSR |= rs1` |
| csrrc | 0x73 | I | 011 | csr | `rd = CSR; CSR &= ~rs1` |
| csrrwi | 0x73 | I | 101 | csr | `rd = CSR; CSR = imm[4:0]` |
| csrrsi | 0x73 | I | 110 | csr | `rd = CSR; CSR |= imm[4:0]` |
| csrrci | 0x73 | I | 111 | csr | `rd = CSR; CSR &= ~imm[4:0]` |
| fence | 0x0F | I | 000 | 0x000 | Full barrier (fixed encoding) |

### 2.3.1 Arithmetic Instructions

#### `add rd, rs1, rs2`
- **Operation:** `rd = rs1 + rs2`
- **Wraparound:** 64-bit two’s-complement wraparound on overflow.

#### `sub rd, rs1, rs2`
- **Operation:** `rd = rs1 - rs2`
- **Wraparound:** 64-bit two’s-complement wraparound on overflow.

#### `mul rd, rs1, rs2`
- **Operation:** `rd = (rs1 × rs2) mod 2^64`
- **Signedness:** Signed multiplication.

#### `mulh rd, rs1, rs2`
- **Operation:** Upper 64 bits of signed 128-bit product.

#### `mulhu rd, rs1, rs2`
- **Operation:** Upper 64 bits of unsigned 128-bit product.

#### `div rd, rs1, rs2`
- **Operation:** Signed division: `rd = rs1 / rs2`
- **Division by zero:** `rd = -1` (all ones), consistent across implementations.
- **Overflow:** For `INT64_MIN / -1`, result is `INT64_MIN`.

#### `divu rd, rs1, rs2`
- **Operation:** Unsigned division.
- **Division by zero:** `rd = 2^64 - 1`.

#### `rem rd, rs1, rs2`
- **Operation:** Signed remainder: `rd = rs1 % rs2`
- **Division by zero:** `rd = rs1`.

#### `remu rd, rs1, rs2`
- **Operation:** Unsigned remainder.
- **Division by zero:** `rd = rs1`.

#### `addi rd, rs1, imm`
- **Operation:** `rd = rs1 + imm`
- **Immediate:** Sign-extended.

### 2.3.2 Logical Instructions

#### `and rd, rs1, rs2`
- **Operation:** `rd = rs1 & rs2`

#### `or rd, rs1, rs2`
- **Operation:** `rd = rs1 | rs2`

#### `xor rd, rs1, rs2`
- **Operation:** `rd = rs1 ^ rs2`


#### `andi rd, rs1, imm`
- **Operation:** `rd = rs1 & imm`
- **Immediate:** Sign-extended.

#### `ori rd, rs1, imm`
- **Operation:** `rd = rs1 | imm`
- **Immediate:** Sign-extended.

#### `xori rd, rs1, imm`
- **Operation:** `rd = rs1 ^ imm`
- **Immediate:** Sign-extended.

### 2.3.3 Shift Instructions

#### `sll rd, rs1, rs2`
- **Operation:** Logical left shift by lower 6 bits of `rs2`.

#### `srl rd, rs1, rs2`
- **Operation:** Logical right shift by lower 6 bits of `rs2`.

#### `sra rd, rs1, rs2`
- **Operation:** Arithmetic right shift by lower 6 bits of `rs2`.

#### `slli rd, rs1, imm`
- **Operation:** Left shift by immediate (0–63).

#### `srli rd, rs1, imm`
- **Operation:** Logical right shift by immediate (0–63).

#### `srai rd, rs1, imm`
- **Operation:** Arithmetic right shift by immediate (0–63).

### 2.3.4 Comparison Instructions

#### `slt rd, rs1, rs2`
- **Operation:** `rd = (rs1 < rs2) ? 1 : 0` (signed)

#### `sltu rd, rs1, rs2`
- **Operation:** `rd = (rs1 < rs2) ? 1 : 0` (unsigned)

#### `slti rd, rs1, imm`
- **Operation:** `rd = (rs1 < imm) ? 1 : 0` (signed)

#### `sltiu rd, rs1, imm`
- **Operation:** `rd = (rs1 < imm) ? 1 : 0` (unsigned)

### 2.3.5 Load Instructions

All loads compute an effective address `EA = rs1 + imm` and load from memory with natural alignment.

#### `ldb rd, imm(rs1)`
- **Operation:** Load signed byte, sign-extend to 64 bits.

#### `ldbu rd, imm(rs1)`
- **Operation:** Load unsigned byte, zero-extend to 64 bits.

#### `ldh rd, imm(rs1)`
- **Operation:** Load signed halfword (16-bit), sign-extend.

#### `ldhu rd, imm(rs1)`
- **Operation:** Load unsigned halfword, zero-extend.

#### `ldw rd, imm(rs1)`
- **Operation:** Load signed word (32-bit), sign-extend.

#### `ldwu rd, imm(rs1)`
- **Operation:** Load unsigned word, zero-extend.

#### `ld rd, imm(rs1)`
- **Operation:** Load doubleword (64-bit).

### 2.3.6 Store Instructions

All stores compute `EA = rs1 + imm` and store the low bits of `rs2`.

#### `stb rs2, imm(rs1)`
- **Operation:** Store byte (8-bit).

#### `sth rs2, imm(rs1)`
- **Operation:** Store halfword (16-bit).

#### `stw rs2, imm(rs1)`
- **Operation:** Store word (32-bit).

#### `st rs2, imm(rs1)`
- **Operation:** Store doubleword (64-bit).

### 2.3.7 Control-Flow Instructions

#### `beq rs1, rs2, imm`
- **Operation:** If `rs1 == rs2`, branch to `PC + imm`.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `bne rs1, rs2, imm`
- **Operation:** If `rs1 != rs2`, branch to `PC + imm`.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `blt rs1, rs2, imm`
- **Operation:** Signed less-than branch.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `bge rs1, rs2, imm`
- **Operation:** Signed greater-or-equal branch.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `bltu rs1, rs2, imm`
- **Operation:** Unsigned less-than branch.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `bgeu rs1, rs2, imm`
- **Operation:** Unsigned greater-or-equal branch.
 - **Range:** ±4 KiB (13-bit immediate, 4-byte aligned).

#### `jal rd, imm`
- **Operation:** `rd = PC + 4; PC = PC + imm`
 - **Range:** ±1 MiB (21-bit immediate, 4-byte aligned).

#### `jalr rd, rs1, imm`
- **Operation:** `rd = PC + 4; PC = (rs1 + imm) & ~3`
- **Note:** Clear bits [1:0] to enforce 4-byte alignment.

### 2.3.8 Atomic Instructions

#### `amoswap.w rd, rs1, rs2`
- **Operation:** Atomically swap 32-bit value at `rs1` with `rs2`.
- **Result:** `rd = sign_extend(MEM32[rs1])`, then `MEM32[rs1] = rs2[31:0]`.
- **Alignment:** `rs1` must be 4-byte aligned.
- **Encoding:** funct7=0000100, funct3=010. Bits `funct7[6:5]` are reserved for future acquire/release semantics and must be 0 in v1.

#### `amoswap.d rd, rs1, rs2`
- **Operation:** Atomically swap 64-bit value at `rs1` with `rs2`.
- **Result:** `rd = MEM64[rs1]`, then `MEM64[rs1] = rs2`.
- **Alignment:** `rs1` must be 8-byte aligned.
- **Encoding:** funct7=0000100, funct3=011. Bits `funct7[6:5]` are reserved for future acquire/release semantics and must be 0 in v1.

Atomic operations are single-copy atomic and do not imply a fence. Use `fence` to enforce ordering.

**v1 limitation:** only `amoswap.w`/`amoswap.d` are defined. There is no `amoadd`/`amoand`/`amoor` or `lr/sc` pair in v1; spinlocks must use swap-and-test loops.

### 2.3.9 System Instructions

#### `ecall`
- **Operation:** Trap to system call handler.

#### `ebreak`
- **Operation:** Trap to debugger/monitor.

#### `mret`
- **Operation:** Return from machine-mode trap. Privilege and interrupt state restored per [deliverables/traps.md](deliverables/traps.md) and [deliverables/csr.md](deliverables/csr.md).

#### `sret`
- **Operation:** Return from supervisor-mode trap. Privilege and interrupt state restored per [deliverables/traps.md](deliverables/traps.md) and [deliverables/csr.md](deliverables/csr.md).

#### `fence`
- **Operation:** Memory ordering barrier. Fixed encoding provides a full barrier as defined in Section 2.4.

### 2.3.10 CSR Instructions

CSR instructions access privileged control/status registers as defined in [deliverables/csr.md](deliverables/csr.md).

- `csrrw rd, csr, rs1` — read CSR into `rd`, write `rs1` into CSR
- `csrrs rd, csr, rs1` — read CSR into `rd`, set bits present in `rs1`
- `csrrc rd, csr, rs1` — read CSR into `rd`, clear bits present in `rs1`
- `csrrwi rd, csr, imm` — immediate variant (imm[4:0])
- `csrrsi rd, csr, imm` — immediate variant (imm[4:0])
- `csrrci rd, csr, imm` — immediate variant (imm[4:0])

### 2.3.11 Pseudo-Instructions (Assembler Only)

These are not encoded as unique opcodes, but expand into base instructions.

- `mov rd, rs1` → `add rd, rs1, r0`
- `li rd, imm` → `addi rd, r0, imm` or `movhi` + `addi`
- `nop` → `add r0, r0, r0`
- `not rd, rs1` → `xori rd, rs1, -1`
- `ret` → `jalr r0, ra, 0`

### 2.3.12 Notes on `movhi` / `movpc`

The base ISA replaces `lui/auipc` with `movhi` and `movpc` (U-type). **Defined behavior:**

- `movhi rd, imm20` performs `rd = sign_extend(imm20) << 12`.
- `movpc rd, imm20` performs `rd = PC + (sign_extend(imm20) << 12)`.

The lower 12 bits are cleared to zero. This enables standard constant construction using `movhi` + `addi` and PC-relative sequences using `movpc` + `addi`.

Relocation behavior and constant synthesis are defined in [deliverables/relocations.md](deliverables/relocations.md).

---

## 2.4 Memory Model (Base ISA)

MINA adopts **TSO (Total Store Order)** semantics in the base ISA. This is a pragmatic midpoint between strict sequential consistency and fully relaxed models.

### 2.4.1 Ordering Rules

- **Program order preserved** for: Load→Load, Load→Store, Store→Store.
- **Store→Load reordering** is permitted for different addresses (via store buffering).
- A **fence** enforces a full barrier: no memory operation after the fence is observed before any memory operation preceding it.

### 2.4.2 Atomicity and Visibility

- Naturally aligned loads/stores up to 64 bits are **single-copy atomic**.
- Misaligned accesses trap (see Section 2.2.4).
- `amoswap.w`/`amoswap.d` are provided as the base atomic read-modify-write primitives.

## 2.5 Next Steps

- Add worked encoding examples for assembler tests.
- Review CSR map and privilege control policy in [deliverables/csr.md](deliverables/csr.md).
- Specify atomic read-modify-write instructions (if required).
