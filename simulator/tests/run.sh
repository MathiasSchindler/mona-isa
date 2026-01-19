#!/bin/sh
set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
AS="$ROOT/../mina-as/mina-as"
SIM="$ROOT/mina-sim"
OUT="$ROOT/tests/out"

mkdir -p "$OUT"

if [ ! -x "$SIM" ]; then
  make -s -C "$ROOT"
fi

if [ ! -x "$AS" ]; then
  make -s -C "$ROOT/../mina-as"
fi

run_test() {
  name="$1"
  src="$2"
  expected="$3"
  input="$4"
  shift 4
  extra_args="$@"

  for opt in "" "-O"; do
    suffix=""
    if [ -n "$opt" ]; then suffix="-opt"; fi
    elf="$OUT/${name}${suffix}.elf"
    out="$OUT/${name}${suffix}.out"

    $AS $opt $extra_args "$src" -o "$elf"
    if [ -n "$input" ]; then
      printf "%s" "$input" | $SIM "$elf" > "$out" 2>/dev/null
    else
      $SIM "$elf" > "$out" 2>/dev/null
    fi
    cmp -s "$out" "$expected"
    rm -f "$out"
  done

  echo "PASS $name"
}

run_test "hello" "$ROOT/../mina-as/tests/src/hello.s" "$ROOT/tests/expected/hello.txt" ""

run_test "uart-echo" "$ROOT/../mina-as/tests/src/uart-echo.s" "$ROOT/tests/expected/uart-echo.txt" "X"

run_test "csr-test" "$ROOT/../mina-as/tests/src/csr-test.s" "$ROOT/tests/expected/csr-test.txt" ""

run_test "csr-counter-test" "$ROOT/../mina-as/tests/src/csr-counter-test.s" "$ROOT/tests/expected/csr-counter-test.txt" ""

run_test "csr-sstatus-mask-test" "$ROOT/../mina-as/tests/src/csr-sstatus-mask-test.s" "$ROOT/tests/expected/csr-sstatus-mask-test.txt" ""

run_test "trap-test" "$ROOT/../mina-as/tests/src/trap-test.s" "$ROOT/tests/expected/trap-test.txt" ""

run_test "system-ret-deleg-test" "$ROOT/../mina-as/tests/src/system-ret-deleg-test.s" "$ROOT/tests/expected/system-ret-deleg-test.txt" ""

run_test "system-ret-bits-test" "$ROOT/../mina-as/tests/src/system-ret-bits-test.s" "$ROOT/tests/expected/system-ret-bits-test.txt" ""

run_test "interrupt-basic-test" "$ROOT/../mina-as/tests/src/interrupt-basic-test.s" "$ROOT/tests/expected/interrupt-basic-test.txt" ""

run_test "cap-test" "$ROOT/../mina-as/tests/src/cap-test.s" "$ROOT/tests/expected/cap-test.txt" ""

run_test "cap-ops-test" "$ROOT/../mina-as/tests/src/cap-ops-test.s" "$ROOT/tests/expected/cap-ops-test.txt" ""

run_test "cap-fault-perm-test" "$ROOT/../mina-as/tests/src/cap-fault-perm-test.s" "$ROOT/tests/expected/cap-fault-perm-test.txt" ""

run_test "cap-fault-bounds-test" "$ROOT/../mina-as/tests/src/cap-fault-bounds-test.s" "$ROOT/tests/expected/cap-fault-bounds-test.txt" ""

run_test "cap-fault-tag-test" "$ROOT/../mina-as/tests/src/cap-fault-tag-test.s" "$ROOT/tests/expected/cap-fault-tag-test.txt" ""

run_test "cap-fault-sealed-test" "$ROOT/../mina-as/tests/src/cap-fault-sealed-test.s" "$ROOT/tests/expected/cap-fault-sealed-test.txt" ""

run_test "syscall-io" "$ROOT/../mina-as/tests/src/syscall-io.s" "$ROOT/tests/expected/syscall-io.txt" ""

run_test "fence-test" "$ROOT/../mina-as/tests/src/fence-test.s" "$ROOT/tests/expected/fence-test.txt" ""

run_test "factorial-test" "$ROOT/../mina-as/tests/src/factorial-test.s" "$ROOT/tests/expected/factorial-test.txt" ""

run_test "fib-test" "$ROOT/../mina-as/tests/src/fib-test.s" "$ROOT/tests/expected/fib-test.txt" ""

run_test "prime-test" "$ROOT/../mina-as/tests/src/prime-test.s" "$ROOT/tests/expected/prime-test.txt" ""

run_test "prime-test2" "$ROOT/../mina-as/tests/src/prime-test2.s" "$ROOT/tests/expected/prime-test2.txt" ""

run_test "mext-test" "$ROOT/../mina-as/tests/src/mext-test.s" "$ROOT/tests/expected/mext-test.txt" ""

run_test "illegal-shift-test" "$ROOT/../mina-as/tests/src/illegal-shift-test.s" "$ROOT/tests/expected/illegal-shift-test.txt" ""

run_test "illegal-insn-test" "$ROOT/../mina-as/tests/src/illegal-insn-test.s" "$ROOT/tests/expected/illegal-insn-test.txt" ""

run_test "misaligned-load-test" "$ROOT/../mina-as/tests/src/misaligned-load-test.s" "$ROOT/tests/expected/misaligned-load-test.txt" ""

run_test "misaligned-store-test" "$ROOT/../mina-as/tests/src/misaligned-store-test.s" "$ROOT/tests/expected/misaligned-store-test.txt" ""

run_test "reverse-test" "$ROOT/../mina-as/tests/src/reverse-test.s" "$ROOT/tests/expected/reverse-test.txt" ""

run_test "palindrome-test" "$ROOT/../mina-as/tests/src/palindrome-test.s" "$ROOT/tests/expected/palindrome-test.txt" ""

run_test "tensor-basic-test" "$ROOT/../mina-as/tests/src/tensor-basic-test.s" "$ROOT/tests/expected/tensor-basic-test.txt" ""

run_test "tensor-fmt-test" "$ROOT/../mina-as/tests/src/tensor-fmt-test.s" "$ROOT/tests/expected/tensor-fmt-test.txt" ""

run_test "tensor-stride0-test" "$ROOT/../mina-as/tests/src/tensor-stride0-test.s" "$ROOT/tests/expected/tensor-stride0-test.txt" ""

run_test "tensor-naninf-test" "$ROOT/../mina-as/tests/src/tensor-naninf-test.s" "$ROOT/tests/expected/tensor-naninf-test.txt" ""

run_test "tensor-sat-test" "$ROOT/../mina-as/tests/src/tensor-sat-test.s" "$ROOT/tests/expected/tensor-sat-test.txt" ""

run_test "tensor-illegal-fmt-test" "$ROOT/../mina-as/tests/src/tensor-illegal-fmt-test.s" "$ROOT/tests/expected/tensor-illegal-fmt-test.txt" ""

run_test "amo-test" "$ROOT/../mina-as/tests/src/amo-test.s" "$ROOT/tests/expected/amo-test.txt" ""

run_test "abi-test" "$ROOT/../mina-as/tests/src/abi-test.s" "$ROOT/tests/expected/abi-test.txt" ""

run_test "abi-stack-test" "$ROOT/../mina-as/tests/src/abi-stack-test.s" "$ROOT/tests/expected/abi-stack-test.txt" ""

run_test "elf-layout-test" "$ROOT/../mina-as/tests/src/elf-layout-test.s" "$ROOT/tests/expected/elf-layout-test.txt" "" \
  --text-base 0x1000 --data-base 0x3000 --bss-base 0x4000 --segment-align 0x1000

echo "ALL TESTS PASS"