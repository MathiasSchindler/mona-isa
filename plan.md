# MINA
## Minimal INstruction Architecture

*A Minimalist Processor Architecture for Linux and AI Workloads*

Design Document and Implementation Roadmap

Version 0.1 — January 2026

---

## 1. Vision and Goals

MINA is a clean-slate processor architecture designed with two seemingly contradictory goals: **radical minimalism** in the base instruction set and **first-class support for modern AI inference**. The thesis is that these goals are not in tension—most complexity in existing architectures stems from historical baggage, not fundamental requirements.

### 1.1 Design Philosophy

The architecture follows several core principles:

- **Orthogonality over convenience:** Every instruction should do one thing. Convenience instructions are the compiler's job, not the ISA's.
- **Human readability matters:** Assembly should read almost like pseudocode. The 68000 proved this is achievable without sacrificing performance.
- **Security by design:** Capability-based memory access and hardware bounds checking are not extensions—they are the memory model.
- **AI as first-class citizen:** Low-precision tensor operations are not bolted on; they are designed alongside the scalar core.
- **Formal verifiability:** The ISA should be small enough to formally verify the core implementation.

### 1.2 Target Use Cases

MINA targets three primary scenarios:

1. **Minimal Linux systems:** Embedded devices, IoT endpoints, and resource-constrained platforms where a full x86 or ARM implementation is overkill.
2. **Edge AI inference:** Running quantized neural networks (INT8, FP8, FP4) efficiently without requiring a separate NPU.
3. **Teaching and research:** An architecture intended to be understandable end-to-end, yet capable enough to run real software.

### 1.3 Non-Goals

Explicitly out of scope for the initial design:

- High-performance computing or server workloads
- Binary compatibility with any existing architecture
- Graphics processing (though SIMD capabilities may be reusable)
- Out-of-order execution in the initial implementation

---

## 2. Instruction Set Architecture Overview

### 2.1 Register File

MINA uses a clean register model inspired by both RISC-V and the 68000:

| Register | Name | Purpose |
|----------|------|---------|
| r0 | zero | Hardwired to zero (reads always return 0, writes ignored) |
| r1-r15 | general purpose | Fully general-purpose (ABI reserves r15 as `tp`) |
| r16-r23 | a0-a7 | Argument/return registers (also general-purpose) |
| r24-r27 | t0-t3 | Temporaries (caller-saved) |
| r28 | fp | Frame pointer |
| r29 | gp | Global pointer (for PIC) |
| r30 | sp | Stack pointer |
| r31 | ra | Return address |

All registers are 64-bit. The naming convention (a0-a7, t0-t3, etc.) is purely for ABI convenience—at the hardware level, r1-r31 are identical.

For authoritative ABI naming, register roles, and calling conventions, see [deliverables/abi.md](deliverables/abi.md). The ISA definition is in [deliverables/isa.md](deliverables/isa.md). Privilege control and CSR definitions are in [deliverables/csr.md](deliverables/csr.md).

### 2.2 Instruction Encoding

MINA uses a fixed 32-bit instruction width. This simplifies fetch logic and instruction cache design. The encoding is divided into six formats:

a) **R-type:** Register-register operations (add, sub, and, or, xor, shifts)
b) **I-type:** Immediate operations and loads (addi, andi, ld, etc.)
c) **S-type:** Store operations
d) **B-type:** Conditional branches
e) **U-type:** Upper immediates (`movhi`, `movpc`)
f) **J-type:** Direct jumps (`jal`)

**Design decision:** Unlike RISC-V, MINA does not support compressed (16-bit) instructions in the base ISA. This trades code density for implementation simplicity. A future extension may add compressed instructions for embedded targets.

Formal encoding details and immediate semantics are defined in [deliverables/isa.md](deliverables/isa.md).

### 2.3 Base Integer Instructions

The complete base integer ISA consists of more than 40 instructions:

**Arithmetic:** add, sub, mul, div, rem (signed and unsigned variants)
**Logic:** and, or, xor, sll, srl, sra
**Comparison:** slt, sltu (set less than)
**Memory:** ld, st (with byte/half/word/double variants)
**Atomic:** amoswap.w, amoswap.d
**Control:** beq, bne, blt, bge, jal, jalr
**System:** ecall, ebreak, fence, CSR read/write (see [deliverables/csr.md](deliverables/csr.md))

This list is a high-level summary. The authoritative base ISA instruction list and encodings are in [deliverables/isa.md](deliverables/isa.md).

This is deliberately smaller than RISC-V's RV64I (which has ~50 instructions). Instructions like *lui* and *auipc* are replaced by *movhi* and *movpc* with clearer semantics.

### 2.4 Tensor Extension (MINA-T)

The tensor extension adds support for low-precision matrix operations essential for neural network inference. Unlike general-purpose vector extensions (like RISC-V V), MINA-T focuses narrowly on the operations that dominate ML workloads.

The authoritative MINA-T instruction definitions and encodings live in [deliverables/mina-t.md](deliverables/mina-t.md).

#### 2.4.1 Tensor Registers

MINA-T adds 8 tensor registers (tr0-tr7), each holding a 16×16 tile of elements. The element type is configurable per-register:

| Format | Bits/Element | Elements per Tile |
|--------|--------------|-------------------|
| FP32 | 32 | 256 (16×16) |
| FP16 / BF16 | 16 | 256 (16×16) |
| FP8 (E4M3) | 8 | 256 (16×16) |
| FP8 (E5M2) | 8 | 256 (16×16) |
| INT8 | 8 | 256 (16×16) |
| FP4 (E2M1) | 4 | 256 (16×16) |

Each tensor register requires 1024 bytes of storage at maximum precision (FP32). For lower precisions, only the necessary bits are used; the rest is reserved.

#### 2.4.2 Tensor Instructions

The tensor extension defines a minimal set of 9 instructions—the minimum necessary for efficient inference:

1. **tld tr, (rs1), stride:** Load tile from memory with configurable stride
2. **tst tr, (rs1), stride:** Store tile to memory
3. **tadd tr1, tr2, tr3:** Element-wise add: tr1 = tr2 + tr3
4. **tmma tr1, tr2, tr3:** Matrix multiply-accumulate: tr1 += tr2 × tr3
5. **tact tr, func:** Apply activation function (ReLU, GELU, SiLU, EXP, RECIP) element-wise
6. **tcvt tr1, tr2, fmt:** Convert between formats (e.g., FP8 to FP16)
7. **tzero tr:** Zero a tensor register
8. **tred tr, rd, op:** Reduce tile to scalar (sum, max, min)
9. **tscale tr, rs:** Scale all elements by scalar

**Design rationale:** This minimal set covers 90% of inference operations. Notably absent are element-wise operations other than activation functions—these are synthesized via scalar code or deferred to future extensions. The *tmma* instruction is the workhorse; everything else supports data movement and format conversion.

---

## 3. Implementation Roadmap

Development follows a three-strand approach, with work rotating between strands as blocking dependencies arise. This mirrors the bootstrapping strategy used in self-hosting compiler projects.

### 3.1 Strand A: ISA Specification and Toolchain

The specification is the source of truth. Ambiguities discovered in other strands feed back into spec refinement.

#### Milestone A1: Informal Specification (Week 1-2)
Deliverable: Markdown document specifying instruction encodings, register file, and basic memory model.

*Test:* Manual review. Can a competent engineer implement an instruction decoder from this document alone?

#### Milestone A2: Assembler (Week 3-5)
Deliverable: A minimal assembler (mina-as) that reads assembly and outputs raw binary. No linker, no relocations—just direct assembly.

*Test:* Assemble a trivial program (load immediate, add, store) and verify byte-level output matches hand-calculated encoding.

#### Milestone A3: Disassembler (Week 6)
Deliverable: mina-objdump reads binaries and produces assembly.

*Test:* Round-trip: assemble → disassemble → assemble should produce identical binaries.

#### Milestone A4: Linker (Week 7-9)
Deliverable: mina-ld handles relocations, symbol resolution, and produces ELF binaries.

*Test:* Link two object files with cross-references; verify symbol addresses are correctly patched.

#### Milestone A5: Formal Specification (Week 10-12)
Deliverable: Sail or similar formal spec that can generate reference emulator.

*Test:* Generated emulator produces identical results to hand-written emulator (Strand B) on test suite.

### 3.2 Strand B: Emulator and Software Stack

#### Milestone B1: Minimal Emulator (Week 2-4)
Deliverable: Instruction interpreter that can execute basic programs. No OS, no I/O beyond memory-mapped UART.

*Test:* "Hello World" via UART emulation. Program loads string address, loops through bytes, writes to UART register.

#### Milestone B2: Syscall Interface (Week 5-6)
Deliverable: Linux-like syscall emulation (read, write, exit, mmap basics).

*Test:* Run a program that reads from stdin, processes input, writes to stdout.

#### Milestone B3: C Cross-Compiler (Week 7-12)
Deliverable: LLVM or GCC backend producing MINA code. Initially -O0 only.

*Test:* Compile and run a simple C program (factorial, linked list manipulation) on emulator.

#### Milestone B4: Userland Basics (Week 13-16)
Deliverable: Minimal libc (or musl port), basic coreutils (cat, echo, ls).

*Test:* Boot emulator with initramfs, run shell commands.

#### Milestone B5: Minimal Kernel Syscall Layer (Week 17-24)
Deliverable: A small kernel that provides a Linux-compatible syscall surface for a minimal userland. Focus only on the ~20 core syscalls needed to run simple C programs.

*Test:* Boot to shell, run userspace programs, basic file I/O works via the syscall layer.

#### Milestone B6: Tensor Extension Emulation (Week 12-14, parallel)
Deliverable: Emulator supports MINA-T instructions with software implementation.

*Test:* Run a simple matrix multiplication using tensor instructions; verify against scalar reference.

#### Milestone B7: ML Inference Demo (Week 20-24, parallel)
Deliverable: Run quantized MNIST or TinyLlama inference using tensor extension.

*Test:* Inference produces correct output; performance measured in emulated cycles.

### 3.3 Strand C: Hardware Implementation

#### Milestone C1: Single-Cycle Core (Week 4-8)
Deliverable: Verilog/SystemVerilog implementation of base integer ISA. Single-cycle execution, no pipelining.

*Test:* Run assembler test suite in simulation (Verilator); all tests pass.

#### Milestone C2: FPGA Bring-up (Week 9-10)
Deliverable: Core running on an FPGA platform sized to the design’s resource needs (UART I/O working). Board selection should follow the resource requirements, not precede them.

*Test:* "Hello World" transmitted over physical UART.

#### Milestone C3: Pipelined Core (Week 11-16)
Deliverable: 5-stage pipeline (IF, ID, EX, MEM, WB) with hazard detection and forwarding.

*Test:* Dhrystone benchmark runs correctly; CPI measured.

#### Milestone C4: Memory System (Week 17-20)
Deliverable: Instruction cache, data cache, DRAM controller interface.

*Test:* Run memory-intensive benchmarks; cache hit rates measured.

#### Milestone C5: Privilege Modes (Week 21-24)
Deliverable: Machine/Supervisor/User modes, interrupt controller, MMU.

*Test:* Linux kernel boots on FPGA.

#### Milestone C6: Tensor Unit (Week 20-28, parallel)
Deliverable: Hardware implementation of MINA-T. Systolic array for matrix multiply.

*Test:* ML inference demo runs on FPGA; speedup measured vs. scalar emulation.

---

## 4. Testing Strategy

Testing follows a layered approach, with each layer building on the previous.

### 4.1 Unit Tests

Every instruction has a dedicated test that verifies correct behavior in isolation. Tests are written in assembly with expected register/memory state annotated in comments. A test harness compares actual vs. expected state after execution.

### 4.2 Integration Tests

Small programs (~50-200 instructions) that test instruction interactions: function calls with stack manipulation, loops with memory access patterns, interrupt handling sequences.

### 4.3 Compliance Tests

A formal compliance suite derived from the Sail specification. Any implementation (emulator, hardware) must pass 100% of compliance tests.

### 4.4 Stress Tests

Randomized instruction sequences (constrained random generation) to find edge cases not covered by manual tests. Particularly important for the tensor extension where numerical precision issues may lurk.

### 4.5 Differential Testing

The emulator serves as the reference implementation. Any program that produces different results on hardware vs. emulator is a bug (in one or the other). This catches subtle timing issues and uninitialized state problems.

### 4.6 Real-World Benchmarks

Dhrystone, CoreMark, and eventually SPEC-like workloads validate that the architecture performs adequately on realistic code. For the tensor extension: MLPerf Tiny benchmarks.

---

## 5. Open Design Questions

Several architectural decisions remain open pending prototyping experience:

### 5.1 Instruction Encoding Trade-offs

v1 defines immediates as **sign-extended by default** (including logical-immediate forms). A future extension may add explicit zero-extending immediate variants if code generation needs warrant it.

### 5.2 Memory Model

MINA adopts **TSO (Total Store Order)** as the base memory model. The formal definition lives in the ISA specification.

### 5.3 FP8 Format Selection

E4M3 vs. E5M2: E4M3 has more mantissa bits (better precision for weights), E5M2 has more exponent bits (better range for gradients). Current decision: support both, with E4M3 as default for inference.

### 5.4 Tensor Tile Size

16×16 is chosen as a balance between utilization (most matrix dimensions are multiples of 16) and silicon area. Alternatives: 8×8 (smaller, more flexible) or 32×32 (higher throughput, worse utilization for small matrices). This requires prototyping to validate.

### 5.5 Capability Architecture

Capability-based memory access is mandatory in the base model; legacy mode is allowed only for bootstrap. The concrete capability format and rules are specified in the capability deliverable.

---

## 6. Success Criteria

A minimal kernel boots on FPGA, provides a Linux-compatible syscall surface sufficient for a small C userland, runs a shell, and can execute a quantized neural network using hardware-accelerated tensor operations. The entire system—from RTL to userspace—should be understandable by a single person.
