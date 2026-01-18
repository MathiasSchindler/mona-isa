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

$AS "$ROOT/../mina-as/tests/src/hello.s" -o "$OUT/hello.elf"
$SIM "$OUT/hello.elf" > "$OUT/hello.out" 2>/dev/null
cmp -s "$OUT/hello.out" "$ROOT/tests/expected/hello.txt"
rm -f "$OUT/hello.out"
echo "PASS hello"

$AS "$ROOT/../mina-as/tests/src/uart-echo.s" -o "$OUT/uart-echo.elf"
printf 'X' | $SIM "$OUT/uart-echo.elf" > "$OUT/uart-echo.out" 2>/dev/null
cmp -s "$OUT/uart-echo.out" "$ROOT/tests/expected/uart-echo.txt"
rm -f "$OUT/uart-echo.out"
echo "PASS uart-echo"

$AS "$ROOT/../mina-as/tests/src/csr-test.s" -o "$OUT/csr-test.elf"
$SIM "$OUT/csr-test.elf" > "$OUT/csr-test.out" 2>/dev/null
cmp -s "$OUT/csr-test.out" "$ROOT/tests/expected/csr-test.txt"
rm -f "$OUT/csr-test.out"
echo "PASS csr-test"

$AS "$ROOT/../mina-as/tests/src/csr-counter-test.s" -o "$OUT/csr-counter-test.elf"
$SIM "$OUT/csr-counter-test.elf" > "$OUT/csr-counter-test.out" 2>/dev/null
cmp -s "$OUT/csr-counter-test.out" "$ROOT/tests/expected/csr-counter-test.txt"
rm -f "$OUT/csr-counter-test.out"
echo "PASS csr-counter-test"

$AS "$ROOT/../mina-as/tests/src/csr-sstatus-mask-test.s" -o "$OUT/csr-sstatus-mask-test.elf"
$SIM "$OUT/csr-sstatus-mask-test.elf" > "$OUT/csr-sstatus-mask-test.out" 2>/dev/null
cmp -s "$OUT/csr-sstatus-mask-test.out" "$ROOT/tests/expected/csr-sstatus-mask-test.txt"
rm -f "$OUT/csr-sstatus-mask-test.out"
echo "PASS csr-sstatus-mask-test"

$AS "$ROOT/../mina-as/tests/src/trap-test.s" -o "$OUT/trap-test.elf"
$SIM "$OUT/trap-test.elf" > "$OUT/trap-test.out" 2>/dev/null
cmp -s "$OUT/trap-test.out" "$ROOT/tests/expected/trap-test.txt"
rm -f "$OUT/trap-test.out"
echo "PASS trap-test"

$AS "$ROOT/../mina-as/tests/src/system-ret-deleg-test.s" -o "$OUT/system-ret-deleg-test.elf"
$SIM "$OUT/system-ret-deleg-test.elf" > "$OUT/system-ret-deleg-test.out" 2>/dev/null
cmp -s "$OUT/system-ret-deleg-test.out" "$ROOT/tests/expected/system-ret-deleg-test.txt"
rm -f "$OUT/system-ret-deleg-test.out"
echo "PASS system-ret-deleg-test"

$AS "$ROOT/../mina-as/tests/src/cap-test.s" -o "$OUT/cap-test.elf"
$SIM "$OUT/cap-test.elf" > "$OUT/cap-test.out" 2>/dev/null
cmp -s "$OUT/cap-test.out" "$ROOT/tests/expected/cap-test.txt"
rm -f "$OUT/cap-test.out"
echo "PASS cap-test"

$AS "$ROOT/../mina-as/tests/src/cap-ops-test.s" -o "$OUT/cap-ops-test.elf"
$SIM "$OUT/cap-ops-test.elf" > "$OUT/cap-ops-test.out" 2>/dev/null
cmp -s "$OUT/cap-ops-test.out" "$ROOT/tests/expected/cap-ops-test.txt"
rm -f "$OUT/cap-ops-test.out"
echo "PASS cap-ops-test"

$AS "$ROOT/../mina-as/tests/src/cap-fault-perm-test.s" -o "$OUT/cap-fault-perm-test.elf"
$SIM "$OUT/cap-fault-perm-test.elf" > "$OUT/cap-fault-perm-test.out" 2>/dev/null
cmp -s "$OUT/cap-fault-perm-test.out" "$ROOT/tests/expected/cap-fault-perm-test.txt"
rm -f "$OUT/cap-fault-perm-test.out"
echo "PASS cap-fault-perm-test"

$AS "$ROOT/../mina-as/tests/src/cap-fault-bounds-test.s" -o "$OUT/cap-fault-bounds-test.elf"
$SIM "$OUT/cap-fault-bounds-test.elf" > "$OUT/cap-fault-bounds-test.out" 2>/dev/null
cmp -s "$OUT/cap-fault-bounds-test.out" "$ROOT/tests/expected/cap-fault-bounds-test.txt"
rm -f "$OUT/cap-fault-bounds-test.out"
echo "PASS cap-fault-bounds-test"

$AS "$ROOT/../mina-as/tests/src/cap-fault-tag-test.s" -o "$OUT/cap-fault-tag-test.elf"
$SIM "$OUT/cap-fault-tag-test.elf" > "$OUT/cap-fault-tag-test.out" 2>/dev/null
cmp -s "$OUT/cap-fault-tag-test.out" "$ROOT/tests/expected/cap-fault-tag-test.txt"
rm -f "$OUT/cap-fault-tag-test.out"
echo "PASS cap-fault-tag-test"

$AS "$ROOT/../mina-as/tests/src/cap-fault-sealed-test.s" -o "$OUT/cap-fault-sealed-test.elf"
$SIM "$OUT/cap-fault-sealed-test.elf" > "$OUT/cap-fault-sealed-test.out" 2>/dev/null
cmp -s "$OUT/cap-fault-sealed-test.out" "$ROOT/tests/expected/cap-fault-sealed-test.txt"
rm -f "$OUT/cap-fault-sealed-test.out"
echo "PASS cap-fault-sealed-test"

$AS "$ROOT/../mina-as/tests/src/syscall-io.s" -o "$OUT/syscall-io.elf"
$SIM "$OUT/syscall-io.elf" > "$OUT/syscall-io.out" 2>/dev/null
cmp -s "$OUT/syscall-io.out" "$ROOT/tests/expected/syscall-io.txt"
rm -f "$OUT/syscall-io.out"
echo "PASS syscall-io"

$AS "$ROOT/../mina-as/tests/src/fence-test.s" -o "$OUT/fence-test.elf"
$SIM "$OUT/fence-test.elf" > "$OUT/fence-test.out" 2>/dev/null
cmp -s "$OUT/fence-test.out" "$ROOT/tests/expected/fence-test.txt"
rm -f "$OUT/fence-test.out"
echo "PASS fence-test"

$AS "$ROOT/../mina-as/tests/src/factorial-test.s" -o "$OUT/factorial-test.elf"
$SIM "$OUT/factorial-test.elf" > "$OUT/factorial-test.out" 2>/dev/null
cmp -s "$OUT/factorial-test.out" "$ROOT/tests/expected/factorial-test.txt"
rm -f "$OUT/factorial-test.out"
echo "PASS factorial-test"

$AS "$ROOT/../mina-as/tests/src/fib-test.s" -o "$OUT/fib-test.elf"
$SIM "$OUT/fib-test.elf" > "$OUT/fib-test.out" 2>/dev/null
cmp -s "$OUT/fib-test.out" "$ROOT/tests/expected/fib-test.txt"
rm -f "$OUT/fib-test.out"
echo "PASS fib-test"

$AS "$ROOT/../mina-as/tests/src/prime-test.s" -o "$OUT/prime-test.elf"
$SIM "$OUT/prime-test.elf" > "$OUT/prime-test.out" 2>/dev/null
cmp -s "$OUT/prime-test.out" "$ROOT/tests/expected/prime-test.txt"
rm -f "$OUT/prime-test.out"
echo "PASS prime-test"

$AS "$ROOT/../mina-as/tests/src/prime-test2.s" -o "$OUT/prime-test2.elf"
$SIM "$OUT/prime-test2.elf" > "$OUT/prime-test2.out" 2>/dev/null
cmp -s "$OUT/prime-test2.out" "$ROOT/tests/expected/prime-test2.txt"
rm -f "$OUT/prime-test2.out"
echo "PASS prime-test2"

$AS "$ROOT/../mina-as/tests/src/mext-test.s" -o "$OUT/mext-test.elf"
$SIM "$OUT/mext-test.elf" > "$OUT/mext-test.out" 2>/dev/null
cmp -s "$OUT/mext-test.out" "$ROOT/tests/expected/mext-test.txt"
rm -f "$OUT/mext-test.out"
echo "PASS mext-test"

$AS "$ROOT/../mina-as/tests/src/illegal-shift-test.s" -o "$OUT/illegal-shift-test.elf"
$SIM "$OUT/illegal-shift-test.elf" > "$OUT/illegal-shift-test.out" 2>/dev/null
cmp -s "$OUT/illegal-shift-test.out" "$ROOT/tests/expected/illegal-shift-test.txt"
rm -f "$OUT/illegal-shift-test.out"
echo "PASS illegal-shift-test"

$AS "$ROOT/../mina-as/tests/src/illegal-insn-test.s" -o "$OUT/illegal-insn-test.elf"
$SIM "$OUT/illegal-insn-test.elf" > "$OUT/illegal-insn-test.out" 2>/dev/null
cmp -s "$OUT/illegal-insn-test.out" "$ROOT/tests/expected/illegal-insn-test.txt"
rm -f "$OUT/illegal-insn-test.out"
echo "PASS illegal-insn-test"

$AS "$ROOT/../mina-as/tests/src/misaligned-load-test.s" -o "$OUT/misaligned-load-test.elf"
$SIM "$OUT/misaligned-load-test.elf" > "$OUT/misaligned-load-test.out" 2>/dev/null
cmp -s "$OUT/misaligned-load-test.out" "$ROOT/tests/expected/misaligned-load-test.txt"
rm -f "$OUT/misaligned-load-test.out"
echo "PASS misaligned-load-test"

$AS "$ROOT/../mina-as/tests/src/misaligned-store-test.s" -o "$OUT/misaligned-store-test.elf"
$SIM "$OUT/misaligned-store-test.elf" > "$OUT/misaligned-store-test.out" 2>/dev/null
cmp -s "$OUT/misaligned-store-test.out" "$ROOT/tests/expected/misaligned-store-test.txt"
rm -f "$OUT/misaligned-store-test.out"
echo "PASS misaligned-store-test"

$AS "$ROOT/../mina-as/tests/src/reverse-test.s" -o "$OUT/reverse-test.elf"
$SIM "$OUT/reverse-test.elf" > "$OUT/reverse-test.out" 2>/dev/null
cmp -s "$OUT/reverse-test.out" "$ROOT/tests/expected/reverse-test.txt"
rm -f "$OUT/reverse-test.out"
echo "PASS reverse-test"

$AS "$ROOT/../mina-as/tests/src/palindrome-test.s" -o "$OUT/palindrome-test.elf"
$SIM "$OUT/palindrome-test.elf" > "$OUT/palindrome-test.out" 2>/dev/null
cmp -s "$OUT/palindrome-test.out" "$ROOT/tests/expected/palindrome-test.txt"
rm -f "$OUT/palindrome-test.out"
echo "PASS palindrome-test"

$AS "$ROOT/../mina-as/tests/src/tensor-basic-test.s" -o "$OUT/tensor-basic-test.elf"
$SIM "$OUT/tensor-basic-test.elf" > "$OUT/tensor-basic-test.out" 2>/dev/null
cmp -s "$OUT/tensor-basic-test.out" "$ROOT/tests/expected/tensor-basic-test.txt"
rm -f "$OUT/tensor-basic-test.out"
echo "PASS tensor-basic-test"

$AS "$ROOT/../mina-as/tests/src/tensor-fmt-test.s" -o "$OUT/tensor-fmt-test.elf"
$SIM "$OUT/tensor-fmt-test.elf" > "$OUT/tensor-fmt-test.out" 2>/dev/null
cmp -s "$OUT/tensor-fmt-test.out" "$ROOT/tests/expected/tensor-fmt-test.txt"
rm -f "$OUT/tensor-fmt-test.out"
echo "PASS tensor-fmt-test"

$AS "$ROOT/../mina-as/tests/src/tensor-stride0-test.s" -o "$OUT/tensor-stride0-test.elf"
$SIM "$OUT/tensor-stride0-test.elf" > "$OUT/tensor-stride0-test.out" 2>/dev/null
cmp -s "$OUT/tensor-stride0-test.out" "$ROOT/tests/expected/tensor-stride0-test.txt"
rm -f "$OUT/tensor-stride0-test.out"
echo "PASS tensor-stride0-test"

$AS "$ROOT/../mina-as/tests/src/amo-test.s" -o "$OUT/amo-test.elf"
$SIM "$OUT/amo-test.elf" > "$OUT/amo-test.out" 2>/dev/null
cmp -s "$OUT/amo-test.out" "$ROOT/tests/expected/amo-test.txt"
rm -f "$OUT/amo-test.out"
echo "PASS amo-test"

$AS "$ROOT/../mina-as/tests/src/abi-test.s" -o "$OUT/abi-test.elf"
$SIM "$OUT/abi-test.elf" > "$OUT/abi-test.out" 2>/dev/null
cmp -s "$OUT/abi-test.out" "$ROOT/tests/expected/abi-test.txt"
rm -f "$OUT/abi-test.out"
echo "PASS abi-test"

$AS "$ROOT/../mina-as/tests/src/abi-stack-test.s" -o "$OUT/abi-stack-test.elf"
$SIM "$OUT/abi-stack-test.elf" > "$OUT/abi-stack-test.out" 2>/dev/null
cmp -s "$OUT/abi-stack-test.out" "$ROOT/tests/expected/abi-stack-test.txt"
rm -f "$OUT/abi-stack-test.out"
echo "PASS abi-stack-test"

$AS --text-base 0x1000 --data-base 0x3000 --bss-base 0x4000 --segment-align 0x1000 "$ROOT/../mina-as/tests/src/elf-layout-test.s" -o "$OUT/elf-layout-test.elf"
$SIM "$OUT/elf-layout-test.elf" > "$OUT/elf-layout-test.out" 2>/dev/null
cmp -s "$OUT/elf-layout-test.out" "$ROOT/tests/expected/elf-layout-test.txt"
rm -f "$OUT/elf-layout-test.out"
echo "PASS elf-layout-test"

echo "ALL TESTS PASS"