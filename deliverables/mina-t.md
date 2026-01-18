# MINA-T — Tensor Extension Specification (Draft)

This document expands Section 2.4 of the plan into a concrete, implementable specification for the MINA Tensor Extension (MINA-T). The focus is on low-precision inference workloads with a deliberately small instruction set.

---

## 1. Overview

MINA-T adds a fixed set of tensor registers and nine tensor instructions that cover the dominant operations for neural network inference:

- Tile load/store
- Element-wise add
- Matrix multiply-accumulate
- Activation functions
- Format conversion
- Reduction
- Scaling
- Zeroing

MINA-T is designed to be implemented as a tightly coupled functional unit attached to the scalar core. The extension is optional at the system level but has no optional features inside itself.

---

## 2. Tensor Registers

### 2.1 Register File

- **Registers:** `tr0`–`tr7` (8 total)
- **Logical shape:** 16×16 tile
- **Maximum storage per register:** 1024 bytes (FP32)

### 2.2 Element Formats

Each tensor register carries an element format tag that affects interpretation and arithmetic behavior. The tag is set by `tcvt` and must be consulted by `tmma`, `tact`, `tred`, and `tscale`.

| Format | Bits/Element | Notes |
|---|---:|---|
| FP32 | 32 | Full precision |
| FP16 | 16 | IEEE FP16 |
| BF16 | 16 | Brain float |
| FP8 (E4M3) | 8 | Default inference format |
| FP8 (E5M2) | 8 | Wider exponent range |
| INT8 | 8 | Signed integer |
| FP4 (E2M1) | 4 | Ultra-low precision |

### 2.3 Layout and Addressing

- Tensor elements are stored in row-major order.
- A tile is conceptually indexed as `tr[x, y]` where `x` and `y` are in `[0, 15]`.
- The physical memory layout for `tld`/`tst` is controlled by a **row stride** in bytes.

---

## 3. Tensor Instructions

All tensor instructions are single operations with no implicit looping. Each instruction operates on one tile.

### 3.0 Rounding, Saturation, and NaNs (Global Rules)

- **FP32/FP16/BF16/FP8/FP4 rounding:** round-to-nearest-even.
- **Subnormal handling:** flush-to-zero for FP8/FP4; preserved for FP16/BF16/FP32.
- **NaN propagation:** quiet NaNs propagate; signaling NaNs are treated as quiet.
- **INT8 saturation:** on overflow, clamp to [-128, 127].

### 3.1 `tld trD, (rs1), stride`

**Load tile from memory**

- **Inputs:** base address `rs1`, row stride in bytes
- **Operation:**
  - For each row `y` in `[0,15]`, load 16 elements from address `rs1 + y * stride` into `trD`.
  - Element width is determined by `trD`'s current format tag.
- **Alignment:** base address must be aligned to element size; misalignment traps.
- **Stride limit:** 12-bit signed immediate (-2048 to +2047 bytes). Larger strides must be synthesized via address arithmetic.

### 3.2 `tst trS, (rs1), stride`

**Store tile to memory**

- **Inputs:** base address `rs1`, row stride in bytes
- **Operation:**
  - For each row `y` in `[0,15]`, store 16 elements from `trS` into `rs1 + y * stride`.
  - Element width is determined by `trS`'s format tag.
- **Alignment:** base address must be aligned to element size; misalignment traps.
- **Stride limit:** 12-bit signed immediate (-2048 to +2047 bytes). Larger strides must be synthesized via address arithmetic.

### 3.3 `tadd trD, trA, trB`

**Element-wise add**

- **Operation:** `trD = trA + trB`
- **Format rules:**
  - `trA` and `trB` must be in the same domain (float or integer).
  - Accumulation occurs in FP32 for FP formats and INT32 for INT8, then rounded/saturated to `trD` format.

### 3.4 `tmma trD, trA, trB`

**Matrix multiply-accumulate**

- **Operation:** `trD = trD + (trA × trB)`
- **Interpretation:** `trA` and `trB` are treated as 16×16 matrices.
- **Accumulation:**
  - For FP formats: accumulation occurs in FP32 internally, then rounded to `trD` format on writeback.
  - For INT8: multiplication occurs in INT16, accumulation in INT32, then saturated/rounded to `trD` format on writeback.
- **Format rules:**
  - `trD` defines the output format.
  - `trA` and `trB` may differ but must be in compatible domains (float or integer). Mixed float/int is illegal and traps.

### 3.5 `tact trD, func`

**Element-wise activation**

- **Supported functions:** `relu`, `gelu`, `silu`, `exp`, `recip`
- **Approximation:**
  - `relu(x) = max(0, x)`
  - `gelu(x)` uses the tanh-based approximation: $0.5x(1 + \tanh(\sqrt{2/\pi}(x + 0.044715x^3)))$
  - $silu(x) = x / (1 + e^{-x})$
  - `exp(x) = e^x`
  - `recip(x) = 1/x` (division-by-zero saturates to +∞ for FP and to 0 for INT8)
    - Rationale: INT8 reciprocal uses a floating approximation and clamps to 0 on division-by-zero to avoid surprising max-int saturation.
- **Operation:** Apply activation to each element of `trD`.
- **Format rules:**
  - For integer formats, activations are applied in FP32 and converted back to `trD` format.
- **Legality:** `rs1` must equal `rd`; otherwise the instruction traps as illegal.

### 3.6 `tcvt trD, trS, fmt`

**Format conversion**

- **Operation:** Convert each element of `trS` to `fmt` and store in `trD`.
- **Rounding:** Round-to-nearest-even for FP; saturating conversion for INT8/FP4.
- **Format tag:** `trD`’s format tag is set to `fmt`.

### 3.7 `tzero trD`

**Zero a tensor register**

- **Operation:** Set all elements of `trD` to zero and preserve its format tag.
- **Legality:** `rs1` must equal `rd`; otherwise the instruction traps as illegal.

### 3.8 `tred trS, rd, op`

**Reduce tile to scalar**

- **Operations:** `sum`, `max`, `min`
- **Result:** Written to scalar `rd` (integer register) in 64-bit format.
- **Accumulation:** FP32 accumulator for FP formats; INT64 accumulator for INT8.

### 3.9 `tscale trD, rs1`

**Scale tile by scalar**

- **Operation:** `trD[i] = trD[i] * rs1` for all elements.
- **Format rules:**
  - `rs1` is treated as FP32 for floating formats, INT64 for integer formats.
  - Result is rounded/saturated to `trD`’s format.

---

## 4. Exceptions and Traps

MINA-T instructions trap under these conditions:

- Illegal format combinations (e.g., float × int in `tmma`)
- Misaligned base address for `tld`/`tst`
- Unsupported `func` or `fmt` encoding

Tensor memory operations are subject to capability checks via DDC (see [deliverables/capabilities.md](deliverables/capabilities.md)).

---

## 4.1 Tensor State and Context Switching

- Tensor state is controlled by `mstatus.TS`/`sstatus.TS` as defined in [deliverables/traps.md](deliverables/traps.md).
- When tensor state is disabled, any tensor instruction traps, allowing lazy save/restore.
- On reset, tensor registers are zeroed and format tags default to **FP32**.

---

## 5. Encoding (Formal)

MINA-T uses a dedicated opcode and reuses the existing R- and I-type layouts from the base ISA. Tensor registers are encoded in the 5-bit `rd/rs1/rs2` fields with the upper two bits set to zero (`tr0`–`tr7`).

### 5.1 Opcode Space

| Group | Opcode (bin) | Opcode (hex) | Formats | Notes |
|---|---|---|---|---|
| TENSOR | 1011011 | 0x5B | R, I | Tensor operations |

### 5.2 Encoding Table

**R-type (opcode 0x5B)**

| Instruction | funct7 | funct3 | rd | rs1 | rs2 | Notes |
|---|---|---|---|---|---|---|
| tadd | 0000000 | 000 | trD | trA | trB | — |
| tmma | 0000001 | 001 | trD | trA | trB | — |

Notes:
- In v1, `funct7` selects the operation class (`0000000` for `tadd`, `0000001` for `tmma`). The remaining `funct3` encodings are reserved for future R-type tensor ops within each class.

**I-type (opcode 0x5B)**

| Instruction | funct3 | rd | rs1 | imm[11:0] | Notes |
|---|---|---|---|---|---|
| tld | 000 | trD | rs1 | stride | stride in bytes (signed) |
| tst | 001 | trS | rs1 | stride | stride in bytes (signed) |
| tact | 010 | trD | trD | func | func in imm[2:0] |
| tcvt | 011 | trD | trS | fmt | fmt in imm[3:0] |
| tzero | 100 | trD | trD | 0 | — |
| tred | 101 | rd | trS | op | op in imm[1:0] |
| tscale | 110 | trD | rs1 | 0 | scalar in `rs1` |

### 5.3 Immediate Encodings

**tact func (imm[2:0])**

| func | Encoding |
|---|---|
| relu | 000 |
| gelu | 001 |
| silu | 010 |
| exp | 011 |
| recip | 100 |

**tcvt fmt (imm[3:0])**

| fmt | Encoding |
|---|---|
| FP32 | 0000 |
| FP16 | 0001 |
| BF16 | 0010 |
| FP8 (E4M3) | 0011 |
| FP8 (E5M2) | 0100 |
| INT8 | 0101 |
| FP4 (E2M1) | 0110 |

**tred op (imm[1:0])**

| op | Encoding |
|---|---|
| sum | 00 |
| max | 01 |
| min | 10 |

Notes:
- `tr` operands are encoded in the low 3 bits of the register field; upper bits must be zero.
- `tact`, `tzero`, and `tcvt` use `rs1` as the source tensor; for `tact` and `tzero`, `rs1` must equal `rd`.
- `tred` writes a scalar result to integer `rd` (not a tensor register) and reads the tensor from `rs1`.

---

## 6. Test Guidance

Minimal compliance tests should include:

- `tld`/`tst` round-trip for each format
- `tadd` correctness vs. scalar reference
- `tmma` correctness vs. scalar reference (FP8, INT8)
- `tcvt` edge cases (overflow, underflow, NaN)
- `tact` correctness for each activation (including exp/recip)
- `tred` sum/max/min for random tiles
- `tscale` with scalar 0, 1, and extreme values
