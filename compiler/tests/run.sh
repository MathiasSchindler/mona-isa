#!/bin/sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT_DIR/minac"
AS="$ROOT_DIR/../mina-as/mina-as"
SIM="$ROOT_DIR/../simulator/mina-sim"
OUT="$ROOT_DIR/tests/out"

mkdir -p "$OUT"

if [ ! -x "$BIN" ]; then
  echo "minac binary not found. Run 'make' in compiler/." >&2
  exit 1
fi

if [ ! -x "$AS" ]; then
  make -s -C "$ROOT_DIR/../mina-as"
fi

if [ ! -x "$SIM" ]; then
  make -s -C "$ROOT_DIR/../simulator"
fi

# c0_smoke.c: should succeed
"$BIN" "$ROOT_DIR/tests/c0_smoke.c" >/dev/null
echo "PASS c0_smoke"

# c0_usage.c: invalid args should fail
if "$BIN" >/dev/null 2>&1; then
  echo "FAIL c0_usage" >&2
  exit 1
fi

echo "PASS c0_usage"

# c1_expr.c: should parse
"$BIN" "$ROOT_DIR/tests/c1_expr.c" >/dev/null
echo "PASS c1_expr"

# c1_paren.c: should parse
"$BIN" "$ROOT_DIR/tests/c1_paren.c" >/dev/null
echo "PASS c1_paren"

# c1_err.c: should fail with error
if "$BIN" "$ROOT_DIR/tests/c1_err.c" >/dev/null 2>&1; then
  echo "FAIL c1_err" >&2
  exit 1
fi
echo "PASS c1_err"

# c2_ir.c: should lower and match IR output
"$BIN" --emit-ir "$ROOT_DIR/tests/c2_ir.c" > "$ROOT_DIR/tests/c2_ir.out"
if ! cmp -s "$ROOT_DIR/tests/c2_ir.out" "$ROOT_DIR/tests/c2_ir.txt"; then
  echo "FAIL c2_ir" >&2
  exit 1
fi
rm -f "$ROOT_DIR/tests/c2_ir.out"
echo "PASS c2_ir"

# c2_ir_err.c: should fail in lowering
if "$BIN" --emit-ir "$ROOT_DIR/tests/c2_ir_err.c" >/dev/null 2>&1; then
  echo "FAIL c2_ir_err" >&2
  exit 1
fi
echo "PASS c2_ir_err"

# c3 asm output checks
"$BIN" --emit-asm "$ROOT_DIR/tests/c3_return.c" > "$OUT/c3_return.s"
if ! cmp -s "$OUT/c3_return.s" "$ROOT_DIR/tests/c3_return.s"; then
  echo "FAIL c3_return_asm" >&2
  exit 1
fi
echo "PASS c3_return_asm"

"$BIN" --emit-asm "$ROOT_DIR/tests/c3_add.c" > "$OUT/c3_add.s"
if ! cmp -s "$OUT/c3_add.s" "$ROOT_DIR/tests/c3_add.s"; then
  echo "FAIL c3_add_asm" >&2
  exit 1
fi
echo "PASS c3_add_asm"

"$BIN" --emit-asm "$ROOT_DIR/tests/c3_mul.c" > "$OUT/c3_mul.s"
if ! cmp -s "$OUT/c3_mul.s" "$ROOT_DIR/tests/c3_mul.s"; then
  echo "FAIL c3_mul_asm" >&2
  exit 1
fi
echo "PASS c3_mul_asm"

# c3 simulator smoke checks (halts on exit)
"$BIN" -o "$OUT/c3_return.elf" "$ROOT_DIR/tests/c3_return.c"
OUT_LOG=$("$SIM" "$OUT/c3_return.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c3_return_sim" >&2; exit 1; }
echo "PASS c3_return_sim"

"$BIN" -o "$OUT/c3_add.elf" "$ROOT_DIR/tests/c3_add.c"
OUT_LOG=$("$SIM" "$OUT/c3_add.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c3_add_sim" >&2; exit 1; }
echo "PASS c3_add_sim"

"$BIN" -o "$OUT/c3_mul.elf" "$ROOT_DIR/tests/c3_mul.c"
OUT_LOG=$("$SIM" "$OUT/c3_mul.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c3_mul_sim" >&2; exit 1; }
echo "PASS c3_mul_sim"

# c4 asm output checks
"$BIN" --emit-asm "$ROOT_DIR/tests/c4_vars.c" > "$OUT/c4_vars.s"
if ! cmp -s "$OUT/c4_vars.s" "$ROOT_DIR/tests/c4_vars.s"; then
  echo "FAIL c4_vars_asm" >&2
  exit 1
fi
echo "PASS c4_vars_asm"

"$BIN" --emit-asm "$ROOT_DIR/tests/c4_reassign.c" > "$OUT/c4_reassign.s"
if ! cmp -s "$OUT/c4_reassign.s" "$ROOT_DIR/tests/c4_reassign.s"; then
  echo "FAIL c4_reassign_asm" >&2
  exit 1
fi
echo "PASS c4_reassign_asm"

# c4 simulator smoke checks
"$BIN" -o "$OUT/c4_vars.elf" "$ROOT_DIR/tests/c4_vars.c"
OUT_LOG=$("$SIM" "$OUT/c4_vars.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c4_vars_sim" >&2; exit 1; }
echo "PASS c4_vars_sim"

"$BIN" -o "$OUT/c4_reassign.elf" "$ROOT_DIR/tests/c4_reassign.c"
OUT_LOG=$("$SIM" "$OUT/c4_reassign.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c4_reassign_sim" >&2; exit 1; }
echo "PASS c4_reassign_sim"

# c5 asm output checks
"$BIN" --emit-asm "$ROOT_DIR/tests/c5_if.c" > "$OUT/c5_if.s"
if ! cmp -s "$OUT/c5_if.s" "$ROOT_DIR/tests/c5_if.s"; then
  echo "FAIL c5_if_asm" >&2
  exit 1
fi
echo "PASS c5_if_asm"

"$BIN" --emit-asm "$ROOT_DIR/tests/c5_while.c" > "$OUT/c5_while.s"
if ! cmp -s "$OUT/c5_while.s" "$ROOT_DIR/tests/c5_while.s"; then
  echo "FAIL c5_while_asm" >&2
  exit 1
fi
echo "PASS c5_while_asm"

# c5 simulator smoke checks
"$BIN" -o "$OUT/c5_if.elf" "$ROOT_DIR/tests/c5_if.c"
OUT_LOG=$("$SIM" "$OUT/c5_if.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c5_if_sim" >&2; exit 1; }
echo "PASS c5_if_sim"

"$BIN" -o "$OUT/c5_while.elf" "$ROOT_DIR/tests/c5_while.c"
OUT_LOG=$("$SIM" "$OUT/c5_while.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c5_while_sim" >&2; exit 1; }
echo "PASS c5_while_sim"

# c6 asm output checks
"$BIN" --emit-asm "$ROOT_DIR/tests/c6_call.c" > "$OUT/c6_call.s"
if ! cmp -s "$OUT/c6_call.s" "$ROOT_DIR/tests/c6_call.s"; then
  echo "FAIL c6_call_asm" >&2
  exit 1
fi
echo "PASS c6_call_asm"

"$BIN" --emit-asm "$ROOT_DIR/tests/c6_nested.c" > "$OUT/c6_nested.s"
if ! cmp -s "$OUT/c6_nested.s" "$ROOT_DIR/tests/c6_nested.s"; then
  echo "FAIL c6_nested_asm" >&2
  exit 1
fi
echo "PASS c6_nested_asm"

# c6 simulator smoke checks
"$BIN" -o "$OUT/c6_call.elf" "$ROOT_DIR/tests/c6_call.c"
OUT_LOG=$("$SIM" "$OUT/c6_call.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c6_call_sim" >&2; exit 1; }
echo "PASS c6_call_sim"

"$BIN" -o "$OUT/c6_nested.elf" "$ROOT_DIR/tests/c6_nested.c"
OUT_LOG=$("$SIM" "$OUT/c6_nested.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c6_nested_sim" >&2; exit 1; }
echo "PASS c6_nested_sim"

# C program smoke tests (compile + run)
"$BIN" -o "$OUT/c_prog_fib.elf" "$ROOT_DIR/tests/c_prog_fib.c"
OUT_LOG=$("$SIM" "$OUT/c_prog_fib.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_prog_fib_sim" >&2; exit 1; }
echo "PASS c_prog_fib_sim"

"$BIN" -o "$OUT/c_prog_prime.elf" "$ROOT_DIR/tests/c_prog_prime.c"
OUT_LOG=$("$SIM" "$OUT/c_prog_prime.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_prog_prime_sim" >&2; exit 1; }
echo "PASS c_prog_prime_sim"

# C7 globals and strings
"$BIN" -o "$OUT/c7_global.elf" "$ROOT_DIR/tests/c7_global.c"
OUT_LOG=$("$SIM" "$OUT/c7_global.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c7_global_sim" >&2; exit 1; }
echo "PASS c7_global_sim"

"$BIN" -o "$OUT/c7_string.elf" "$ROOT_DIR/tests/c7_string.c"
OUT_LOG=$("$SIM" "$OUT/c7_string.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c7_string_sim" >&2; exit 1; }
echo "PASS c7_string_sim"

# C8 bin output check
"$BIN" --bin -o "$OUT/c8_return.bin" "$ROOT_DIR/tests/c3_return.c"
OUT_LOG=$("$SIM" "$OUT/c8_return.bin" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c8_bin_sim" >&2; exit 1; }
echo "PASS c8_bin_sim"

# C9 optimizations
"$BIN" -O -o "$OUT/c9_fold.elf" "$ROOT_DIR/tests/c9_fold.c"
OUT_LOG=$("$SIM" "$OUT/c9_fold.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c9_fold_sim" >&2; exit 1; }
echo "PASS c9_fold_sim"

"$BIN" -O -o "$OUT/c9_dead.elf" "$ROOT_DIR/tests/c9_dead.c"
OUT_LOG=$("$SIM" "$OUT/c9_dead.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c9_dead_sim" >&2; exit 1; }
echo "PASS c9_dead_sim"

# C10 tests
"$BIN" -o "$OUT/c10_for_arr.elf" "$ROOT_DIR/tests/c10_for_arr.c"
OUT_LOG=$("$SIM" "$OUT/c10_for_arr.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_for_arr_sim" >&2; exit 1; }
echo "PASS c10_for_arr_sim"

"$BIN" -o "$OUT/c10_switch.elf" "$ROOT_DIR/tests/c10_switch.c"
OUT_LOG=$("$SIM" "$OUT/c10_switch.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_switch_sim" >&2; exit 1; }
echo "PASS c10_switch_sim"

"$BIN" -o "$OUT/c10_logic.elf" "$ROOT_DIR/tests/c10_logic.c"
OUT_LOG=$("$SIM" "$OUT/c10_logic.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_logic_sim" >&2; exit 1; }
echo "PASS c10_logic_sim"

"$BIN" -o "$OUT/c10_ptr.elf" "$ROOT_DIR/tests/c10_ptr.c"
OUT_LOG=$("$SIM" "$OUT/c10_ptr.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_ptr_sim" >&2; exit 1; }
echo "PASS c10_ptr_sim"

"$BIN" -o "$OUT/c10_addr.elf" "$ROOT_DIR/tests/c10_addr.c"
OUT_LOG=$("$SIM" "$OUT/c10_addr.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_addr_sim" >&2; exit 1; }
echo "PASS c10_addr_sim"

"$BIN" -o "$OUT/c10_nested.elf" "$ROOT_DIR/tests/c10_nested.c"
OUT_LOG=$("$SIM" "$OUT/c10_nested.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_nested_sim" >&2; exit 1; }
echo "PASS c10_nested_sim"

"$BIN" -o "$OUT/c10_spill.elf" "$ROOT_DIR/tests/c10_spill.c"
OUT_LOG=$("$SIM" "$OUT/c10_spill.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_spill_sim" >&2; exit 1; }
echo "PASS c10_spill_sim"
