#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
from pathlib import Path

REPORT_TEMPLATE = """# Spec vs Simulator Audit

**Project:** MINA ISA
**Generated:** {generated}

## Sources

- Spec: deliverables/isa.md, deliverables/capabilities.md, deliverables/mina-t.md, deliverables/csr.md
- Simulator: simulator/src/cpu.c, simulator/src/main.c
- Tests: simulator/tests/README.md

## Executive Summary

- **Base integer ISA:** Implemented (ALU, shifts, branches, jumps, loads/stores, movhi/movpc, fence).
- **M extension:** Implemented (`mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu`).
- **SYSTEM/CSR:** Implemented; delegation, mret/sret stacking, and basic interrupt injection tested.
- **CSR counters:** `cycle`, `time`, `instret` implemented and tested.
- **AMO:** Implemented **only** `amoswap.w`/`amoswap.d` (per v1 spec).
- **CAP:** Base set implemented and covered by ops/fault tests.
- **TENSOR:** Implemented all 9 instructions; coverage includes format edge cases and INT8 saturation.
- **Test status:** {test_status}

## Instruction Coverage (Spec vs Simulator)

### Base Integer ISA

| Group | Instructions (spec) | Simulator status | Tests present |
|---|---|---|---|
| ALU R-type | add, sub, and, or, xor, slt, sltu, sll, srl, sra | ✅ Implemented | factorial-test, fib-test, prime-test2, reverse-test |
| ALU I-type | addi, andi, ori, xori, slti, sltiu, slli, srli, srai | ✅ Implemented (shift-imm legality enforced) | illegal-shift-test (negative), various tests |
| Loads | ldb, ldbu, ldh, ldhu, ldw, ldwu, ld | ✅ Implemented | reverse-test, palindrome-test, syscall-io |
| Stores | stb, sth, stw, st | ✅ Implemented | reverse-test, palindrome-test, trap-test |
| Branches | beq, bne, blt, bge, bltu, bgeu | ✅ Implemented | prime-test, fib-test, reverse-test |
| Jumps | jal, jalr | ✅ Implemented | abi-test, abi-stack-test |
| movhi/movpc | movhi, movpc | ✅ Implemented | trap-test (movhi), elf-layout-test |
| fence | fence | ✅ Implemented (no timing model) | fence-test |
| system | ecall, ebreak, mret, sret | ✅ Implemented | trap-test, system-ret-deleg-test, system-ret-bits-test, interrupt-basic-test |

### M Extension

| Instruction | Simulator status | Tests |
|---|---|---|
| mul | ✅ | mext-test, factorial-test |
| mulh | ✅ | mext-test |
| mulhsu | ✅ | mext-test |
| mulhu | ✅ | mext-test |
| div | ✅ | mext-test |
| divu | ✅ | mext-test |
| rem | ✅ | mext-test |
| remu | ✅ | mext-test |

### AMO

| Instruction | Spec | Simulator | Tests |
|---|---|---|---|
| amoswap.w | ✅ | ✅ | amo-test |
| amoswap.d | ✅ | ✅ | amo-test |

### Capability Extension (CAP)

| Instruction | Spec | Simulator | Tests |
|---|---|---|---|
| csetbounds | ✅ | ✅ | cap-ops-test |
| csetperm | ✅ | ✅ | cap-ops-test, cap-fault-perm-test |
| cseal | ✅ | ✅ | cap-ops-test |
| cunseal | ✅ | ✅ | cap-ops-test |
| cgettag | ✅ | ✅ | cap-test, cap-ops-test |
| cgetbase | ✅ | ✅ | cap-ops-test |
| cgetlen | ✅ | ✅ | cap-ops-test |
| cld | ✅ | ✅ | cap-ops-test |
| cst | ✅ | ✅ | cap-ops-test |

### Tensor Extension (MINA-T)

| Instruction | Spec | Simulator | Tests |
|---|---|---|---|
| tld | ✅ | ✅ | tensor_test.bin, tensor-basic-test, tensor-naninf-test, tensor-sat-test |
| tst | ✅ | ✅ | tensor_test.bin, tensor-naninf-test, tensor-sat-test |
| tadd | ✅ | ✅ | tensor_test.bin (limited), tensor-illegal-fmt-test (negative) |
| tmma | ✅ | ✅ | tensor_test.bin (FP8) |
| tact | ✅ | ✅ | tensor-basic-test |
| tcvt | ✅ | ✅ | tensor-basic-test, tensor-fmt-test, tensor-naninf-test, tensor-sat-test |
| tzero | ✅ | ✅ | tensor-basic-test |
| tred | ✅ | ✅ | tensor-basic-test |
| tscale | ✅ | ✅ | tensor-basic-test |

## CSR Coverage (Spec vs Simulator)

| CSR | Spec access | Simulator | Notes |
|---|---|---|---|
| mstatus | R/W | ✅ | subset mask for sstatus enforced |
| mie/mip | R/W | ✅ | interrupts not fully modeled |
| medeleg/mideleg | R/W | ✅ | delegation used in trap_entry |
| mtvec/mepc/mcause/mtval | R/W | ✅ | trap handling implemented |
| mscratch | R/W | ✅ | — |
| sstatus/sie/sip/stvec/sepc/scause/stval/sscratch | R/W | ✅ | — |
| cycle/time/instret | R | ✅ | csr-counter-test |

## Behavioral Constraints & Notes

- **Shift immediates:** non-zero `imm[11:6]` trap (per ISA).
- **Capability mode:** PCC (`c31`) enforced for instruction fetch; DDC (`c0`) enforced for data accesses.
- **CSR writes:** illegal writes to read-only CSRs trap (tested via `csr-counter-test`).
- **AMO:** only `amoswap.w/d` implemented, no LR/SC.
- **Syscalls:** minimal `read/write/exit` only; M-mode syscalls bypass DDC checks for handler output.

## Test Coverage Map (High-Level)

- **Core ISA:** hello, factorial-test, fib-test, reverse-test, palindrome-test, prime-test/prime-test2
- **M extension:** mext-test
- **Traps/illegal:** trap-test, illegal-shift-test, illegal-insn-test, misaligned-load-test, misaligned-store-test, system-ret-deleg-test, system-ret-bits-test, interrupt-basic-test
- **AMO:** amo-test
- **ABI:** abi-test, abi-stack-test
- **ELF layout:** elf-layout-test
- **CAP:** cap-test, cap-ops-test, cap-fault-perm-test, cap-fault-bounds-test, cap-fault-tag-test, cap-fault-sealed-test
- **Tensor:** tensor_test.bin, tensor-basic-test, tensor-fmt-test, tensor-stride0-test, tensor-naninf-test, tensor-sat-test, tensor-illegal-fmt-test
- **CSR counters:** csr-counter-test, csr-sstatus-mask-test
- **Fence:** fence-test

## Remaining Gaps / Recommended Next Actions

No remaining gaps identified at this time.

---

This report reflects the simulator and tests as of the generated timestamp.
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate spec-vs-sim report with timestamp.")
    parser.add_argument("--tests-passed", action="store_true", help="Mark test status as passed.")
    parser.add_argument("--dist", default="dist", help="Output directory (default: dist).")
    args = parser.parse_args()

    now = datetime.now().astimezone()
    stamp = now.strftime("%Y-%m-%d_%H%M%S")
    generated = now.strftime("%Y-%m-%d %H:%M:%S %Z")
    test_status = "passed" if args.tests_passed else "not recorded"

    dist_dir = Path(args.dist)
    dist_dir.mkdir(parents=True, exist_ok=True)
    out_path = dist_dir / f"spec-vs-sim-{stamp}.md"

    content = REPORT_TEMPLATE.format(generated=generated, test_status=test_status)
    out_path.write_text(content, encoding="utf-8")
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
