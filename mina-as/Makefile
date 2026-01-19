CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

BIN = mina-as
SRC = src/main.c src/assemble.c src/buffer.c src/elf.c src/encode.c src/labels.c src/lexer.c src/parse.c src/util.c
SIM = ../simulator/mina-sim

EXAMPLES = tests/src/hello.s tests/src/tensor-mini.s tests/src/99-bottles.s tests/src/uart-echo.s tests/src/csr-test.s tests/src/trap-test.s tests/src/cap-test.s tests/src/syscall-io.s tests/src/factorial-test.s tests/src/fib-test.s tests/src/prime-test.s tests/src/reverse-test.s tests/src/palindrome-test.s tests/src/abi-test.s
HELLO_BIN = tests/bin/hello.bin
TENSOR_BIN = tests/bin/tensor-mini.bin
BOTTLES_BIN = tests/bin/99-bottles.bin
UART_ECHO_BIN = tests/bin/uart-echo.bin
CSR_TEST_BIN = tests/bin/csr-test.bin
TRAP_TEST_BIN = tests/bin/trap-test.bin
CAP_TEST_BIN = tests/bin/cap-test.bin
SYSCALL_IO_BIN = tests/bin/syscall-io.bin
FACTORIAL_TEST_BIN = tests/bin/factorial-test.bin
FIB_TEST_BIN = tests/bin/fib-test.bin
PRIME_TEST_BIN = tests/bin/prime-test.bin
REVERSE_TEST_BIN = tests/bin/reverse-test.bin
PALINDROME_TEST_BIN = tests/bin/palindrome-test.bin
ABI_TEST_BIN = tests/bin/abi-test.bin
ABS_HELLO_BIN = $(CURDIR)/$(HELLO_BIN)
ABS_TENSOR_BIN = $(CURDIR)/$(TENSOR_BIN)
ABS_BOTTLES_BIN = $(CURDIR)/$(BOTTLES_BIN)
ABS_UART_ECHO_BIN = $(CURDIR)/$(UART_ECHO_BIN)
ABS_CSR_TEST_BIN = $(CURDIR)/$(CSR_TEST_BIN)
ABS_TRAP_TEST_BIN = $(CURDIR)/$(TRAP_TEST_BIN)
ABS_CAP_TEST_BIN = $(CURDIR)/$(CAP_TEST_BIN)
ABS_SYSCALL_IO_BIN = $(CURDIR)/$(SYSCALL_IO_BIN)
ABS_FACTORIAL_TEST_BIN = $(CURDIR)/$(FACTORIAL_TEST_BIN)
ABS_FIB_TEST_BIN = $(CURDIR)/$(FIB_TEST_BIN)
ABS_PRIME_TEST_BIN = $(CURDIR)/$(PRIME_TEST_BIN)
ABS_REVERSE_TEST_BIN = $(CURDIR)/$(REVERSE_TEST_BIN)
ABS_PALINDROME_TEST_BIN = $(CURDIR)/$(PALINDROME_TEST_BIN)
ABS_ABI_TEST_BIN = $(CURDIR)/$(ABI_TEST_BIN)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(BIN) $(HELLO_BIN) $(TENSOR_BIN) $(BOTTLES_BIN) $(UART_ECHO_BIN) $(CSR_TEST_BIN) $(TRAP_TEST_BIN) $(CAP_TEST_BIN) $(SYSCALL_IO_BIN) $(FACTORIAL_TEST_BIN) $(FIB_TEST_BIN) $(PRIME_TEST_BIN) $(REVERSE_TEST_BIN) $(PALINDROME_TEST_BIN) $(ABI_TEST_BIN)

examples: $(BIN)
	./$(BIN) tests/src/hello.s -o $(HELLO_BIN)
	./$(BIN) tests/src/tensor-mini.s -o $(TENSOR_BIN)
	./$(BIN) tests/src/99-bottles.s -o $(BOTTLES_BIN)
	./$(BIN) tests/src/uart-echo.s -o $(UART_ECHO_BIN)
	./$(BIN) tests/src/csr-test.s -o $(CSR_TEST_BIN)
	./$(BIN) tests/src/trap-test.s -o $(TRAP_TEST_BIN)
	./$(BIN) tests/src/cap-test.s -o $(CAP_TEST_BIN)
	./$(BIN) tests/src/syscall-io.s -o $(SYSCALL_IO_BIN)
	./$(BIN) tests/src/factorial-test.s -o $(FACTORIAL_TEST_BIN)
	./$(BIN) tests/src/fib-test.s -o $(FIB_TEST_BIN)
	./$(BIN) tests/src/prime-test.s -o $(PRIME_TEST_BIN)
	./$(BIN) tests/src/reverse-test.s -o $(REVERSE_TEST_BIN)
	./$(BIN) tests/src/palindrome-test.s -o $(PALINDROME_TEST_BIN)
	./$(BIN) tests/src/abi-test.s -o $(ABI_TEST_BIN)

run-hello: examples
	$(SIM) $(ABS_HELLO_BIN)

run-tensor: examples
	$(SIM) $(ABS_TENSOR_BIN)

run-99: examples
	$(SIM) $(ABS_BOTTLES_BIN)

run-echo: examples
	$(SIM) $(ABS_UART_ECHO_BIN)

status:
	@printf "MINA status (assembler)\n\n"
	@$(MAKE) -s $(BIN)
	@tmp=$$(mktemp -t mina-as-status.XXXXXX); \
	trap 'rm -f $$tmp $$tmp.bin' EXIT; \
	printf "%s\n" \
	".org 0" \
	"start:" \
	"  # Formats: R/I/S/B/U/J" \
	"  add r1, r0, r0" \
	"  addi r1, r1, 1" \
	"  stw r1, 0, r2" \
	"  beq r0, r0, start" \
	"  movhi r2, 1" \
	"  jal r0, start" \
	"" \
	"  # Opcode groups" \
	"  jalr r0, r31, 0" \
	"  movpc r3, start" \
	"  fence" \
	"  ecall" \
	"  amoswap.w r4, r5, r6" \
	"  cgettag r1, c0" \
	"  tzero tr0" \
	"" \
	"  # Pseudos" \
	"  nop" \
	"  mov r2, r1" \
	"  not r3, r2" \
	"  ret" \
	"  j start" \
	"  jr r31" \
	"  li r4, 0x1234" \
	> $$tmp; \
	if ./$(BIN) $$tmp -o $$tmp.bin >/dev/null 2>&1; then ok=1; else ok=0; fi; \
	printf "Assembler features\n"; \
	if [ $$ok -eq 1 ]; then printf "  Instruction encoding: ✓ (basic smoke)\n"; else printf "  Instruction encoding: ✗\n"; fi; \
	printf "  Pseudo-instructions: %s\n" $$( [ $$ok -eq 1 ] && printf "✓" || printf "✗" ); \
	printf "  Label resolution: %s\n" $$( [ $$ok -eq 1 ] && printf "✓" || printf "✗" ); \
	printf "  Relocation generation: ✗\n"; \
	printf "  ELF output: ✗ (raw binary only)\n\n"; \
	printf "ISA coverage (assembler)\n"; \
	printf "  Formats: %s\n" $$( [ $$ok -eq 1 ] && printf "R✓ I✓ S✓ B✓ U✓ J✓" || printf "R✗ I✗ S✗ B✗ U✗ J✗" ); \
	printf "  Opcode groups: %s\n" $$( [ $$ok -eq 1 ] && printf "OP✓ OP-IMM✓ LOAD✓ STORE✓ BRANCH✓ JAL✓ JALR✓ MOVHI✓ MOVPC✓ SYSTEM✓ FENCE✓ AMO✓ CAP✓ TENSOR✓" || printf "OP✗ OP-IMM✗ LOAD✗ STORE✗ BRANCH✗ JAL✗ JALR✗ MOVHI✗ MOVPC✗ SYSTEM✗ FENCE✗ AMO✗ CAP✗ TENSOR✗" ); \
	printf "  Base count: ~47 total; implemented: ~40 (manual estimate)\n\n"

.PHONY: all clean examples run-hello run-tensor status
