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
- trap-test (ECALL + trap handler)
- cap-test (CAP tag read)
- syscall-io (minimal syscall write)
- factorial-test (loop + multiply)
- fib-test (loop)
- prime-test (trial division)
- prime-test2 (efficient trial division)
- reverse-test (string reverse)
- palindrome-test (string compare)
- amo-test (amoswap.w/d atomics)
- abi-test (call/return + callee-saved)
- abi-stack-test (stack args + alignment)
- elf-layout-test (ELF segments + entry)
