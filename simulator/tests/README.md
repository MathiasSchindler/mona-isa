# Tensor Test

`gen_tensor_test.c` emits a raw binary (`tensor_test.bin`) that exercises:

- FP8 (E4M3) `tld`/`tmma`/`tst`
- FP4 (E2M1) `tld`/`tst` round-trip

The program ends with `ebreak` so the simulator stops after execution.

The binary layout is fixed:

- 0x0000: code
- 0x1000: FP8 A tile
- 0x1100: FP8 B tile
- 0x2000: FP32 output tile
- 0x3000: FP4 input tile
- 0x3080: FP4 output tile

## Self-checking tests

Run:

```sh
make tests
```

This assembles the test sources in `../mina-as/tests/src` and compares stdout
against `tests/expected`.

Current self-checking tests:

- hello (UART TX)
- uart-echo (UART RX/status)
- csr-test (CSR read/write)
- csr-counter-test (cycle/time/instret)
- csr-sstatus-mask-test (sstatus mask)
- trap-test (ECALL + trap handler)
- system-ret-deleg-test (mret/sret + delegation)
- system-ret-bits-test (mret/sret MIE/SIE stacking)
- interrupt-basic-test (mie/mip interrupt injection)
- cap-test (CAP tag read)
- cap-ops-test (CAP ops + cld/cst)
- cap-fault-perm-test (CAP perm fault)
- cap-fault-bounds-test (CAP bounds fault)
- cap-fault-tag-test (CAP tag fault)
- cap-fault-sealed-test (CAP sealed fault)
- syscall-io (minimal syscall write)
- fence-test (fence decode)
- factorial-test (loop + multiply)
- fib-test (loop)
- prime-test (trial division)
- prime-test2 (efficient trial division)
- mext-test (mul/div/rem variants)
- illegal-shift-test (shift-imm encoding legality)
- illegal-insn-test (illegal opcode)
- misaligned-load-test (misaligned load trap)
- misaligned-store-test (misaligned store trap)
- reverse-test (string reverse)
- palindrome-test (string compare)
- tensor-basic-test (tact/tcvt/tzero/tred/tscale)
- tensor-fmt-test (fp4/fp8 conversions)
- tensor-stride0-test (stride edge case)
- tensor-naninf-test (NaN/INF encodings)
- tensor-sat-test (INT8 saturation)
- tensor-illegal-fmt-test (illegal format combo trap)
- amo-test (amoswap.w/d atomics)
- abi-test (call/return + callee-saved)
- abi-stack-test (stack args + alignment)
- elf-layout-test (ELF segments + entry)
