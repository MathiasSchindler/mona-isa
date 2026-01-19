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

# n1_err_recovery.c: should report multiple errors
OUT_LOG=$("$BIN" --max-errors 3 "$ROOT_DIR/tests/n1_err_recovery.c" 2>&1 || true)
if [ -z "$OUT_LOG" ]; then
  echo "FAIL n1_err_recovery" >&2
  exit 1
fi
ERR_COUNT=$(echo "$OUT_LOG" | grep -c "error:")
if [ "$ERR_COUNT" -lt 2 ]; then
  echo "FAIL n1_err_recovery" >&2
  echo "$OUT_LOG" >&2
  exit 1
fi
echo "PASS n1_err_recovery"

# n2_const_init.c: constant expressions in initializers
"$BIN" -o "$OUT/n2_const_init.elf" "$ROOT_DIR/tests/n2_const_init.c"
OUT_LOG=$("$SIM" "$OUT/n2_const_init.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL n2_const_init_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL n2_const_init_sim" >&2; exit 1; }
echo "PASS n2_const_init_sim"

# n3_enum.c: enum declarations and usage
"$BIN" -o "$OUT/n3_enum.elf" "$ROOT_DIR/tests/n3_enum.c"
OUT_LOG=$("$SIM" "$OUT/n3_enum.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL n3_enum_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL n3_enum_sim" >&2; exit 1; }
echo "PASS n3_enum_sim"

# n4_init_struct.c: aggregate initializers
"$BIN" -o "$OUT/n4_init_struct.elf" "$ROOT_DIR/tests/n4_init_struct.c"
OUT_LOG=$("$SIM" "$OUT/n4_init_struct.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL n4_init_struct_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL n4_init_struct_sim" >&2; exit 1; }
echo "PASS n4_init_struct_sim"

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

# label uniqueness in multi-function asm
"$BIN" --emit-asm "$ROOT_DIR/tests/c_label_unique.c" > "$OUT/c_label_unique.s"
LABEL_DUPS=$(grep -E '^\.L[0-9]+:' "$OUT/c_label_unique.s" | sort | uniq -d)
if [ -n "$LABEL_DUPS" ]; then
  echo "FAIL c_label_unique_labels" >&2
  echo "$LABEL_DUPS" >&2
  exit 1
fi
echo "PASS c_label_unique_labels"

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

# Complex C program tests
"$BIN" -o "$OUT/c_prog_structsum.elf" "$ROOT_DIR/tests/c_prog_structsum.c"
OUT_LOG=$("$SIM" "$OUT/c_prog_structsum.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_prog_structsum_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_prog_structsum_sim" >&2; exit 1; }
echo "PASS c_prog_structsum_sim"

"$BIN" -o "$OUT/c_prog_unioncalc.elf" "$ROOT_DIR/tests/c_prog_unioncalc.c"
OUT_LOG=$("$SIM" "$OUT/c_prog_unioncalc.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_prog_unioncalc_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_prog_unioncalc_sim" >&2; exit 1; }
echo "PASS c_prog_unioncalc_sim"

# Stress tests
"$BIN" -o "$OUT/c_stress_control.elf" "$ROOT_DIR/tests/c_stress_control.c"
OUT_LOG=$("$SIM" "$OUT/c_stress_control.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_stress_control_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_stress_control_sim" >&2; exit 1; }
echo "PASS c_stress_control_sim"

"$BIN" -o "$OUT/c_stress_spill.elf" "$ROOT_DIR/tests/c_stress_spill.c"
OUT_LOG=$("$SIM" "$OUT/c_stress_spill.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_stress_spill_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_stress_spill_sim" >&2; exit 1; }
echo "PASS c_stress_spill_sim"

"$BIN" -o "$OUT/c_stress_ptrstruct.elf" "$ROOT_DIR/tests/c_stress_ptrstruct.c"
OUT_LOG=$("$SIM" "$OUT/c_stress_ptrstruct.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_stress_ptrstruct_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_stress_ptrstruct_sim" >&2; exit 1; }
echo "PASS c_stress_ptrstruct_sim"

"$BIN" -o "$OUT/c_stress_byteword.elf" "$ROOT_DIR/tests/c_stress_byteword.c"
OUT_LOG=$("$SIM" "$OUT/c_stress_byteword.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_stress_byteword_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_stress_byteword_sim" >&2; exit 1; }
echo "PASS c_stress_byteword_sim"

"$BIN" -o "$OUT/c_stress_stack.elf" "$ROOT_DIR/tests/c_stress_stack.c"
OUT_LOG=$("$SIM" "$OUT/c_stress_stack.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c_stress_stack_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c_stress_stack_sim" >&2; exit 1; }
echo "PASS c_stress_stack_sim"

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

"$BIN" -o "$OUT/c10_void.elf" "$ROOT_DIR/tests/c10_void.c"
OUT_LOG=$("$SIM" "$OUT/c10_void.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_void_sim" >&2; exit 1; }
echo "PASS c10_void_sim"

"$BIN" -o "$OUT/c10_putchar.elf" "$ROOT_DIR/tests/c10_putchar.c"
OUT_LOG=$("$SIM" "$OUT/c10_putchar.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_putchar_sim" >&2; exit 1; }
echo "PASS c10_putchar_sim"

"$BIN" -o "$OUT/c10_char.elf" "$ROOT_DIR/tests/c10_char.c"
OUT_LOG=$("$SIM" "$OUT/c10_char.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_char_sim" >&2; exit 1; }
echo "PASS c10_char_sim"

"$BIN" -o "$OUT/c10_exit.elf" "$ROOT_DIR/tests/c10_exit.c"
OUT_LOG=$("$SIM" "$OUT/c10_exit.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_exit_sim" >&2; exit 1; }
echo "PASS c10_exit_sim"

"$BIN" -o "$OUT/c10_spill.elf" "$ROOT_DIR/tests/c10_spill.c"
OUT_LOG=$("$SIM" "$OUT/c10_spill.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c10_spill_sim" >&2; exit 1; }
echo "PASS c10_spill_sim"

# C11 tests
"$BIN" -o "$OUT/c11_ptrarith.elf" "$ROOT_DIR/tests/c11_ptrarith.c"
OUT_LOG=$("$SIM" "$OUT/c11_ptrarith.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c11_ptrarith_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c11_ptrarith_sim" >&2; exit 1; }
echo "PASS c11_ptrarith_sim"

"$BIN" -o "$OUT/c11_sizeof.elf" "$ROOT_DIR/tests/c11_sizeof.c"
OUT_LOG=$("$SIM" "$OUT/c11_sizeof.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c11_sizeof_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c11_sizeof_sim" >&2; exit 1; }
echo "PASS c11_sizeof_sim"

# C12 tests
"$BIN" -o "$OUT/c12_struct.elf" "$ROOT_DIR/tests/c12_struct.c"
OUT_LOG=$("$SIM" "$OUT/c12_struct.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c12_struct_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c12_struct_sim" >&2; exit 1; }
echo "PASS c12_struct_sim"

"$BIN" -o "$OUT/c12_union.elf" "$ROOT_DIR/tests/c12_union.c"
OUT_LOG=$("$SIM" "$OUT/c12_union.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c12_union_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c12_union_sim" >&2; exit 1; }
echo "PASS c12_union_sim"

# C13 tests
"$BIN" -o "$OUT/c13_include.elf" "$ROOT_DIR/tests/c13_include.c"
OUT_LOG=$("$SIM" "$OUT/c13_include.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c13_include_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c13_include_sim" >&2; exit 1; }
echo "PASS c13_include_sim"

"$BIN" -o "$OUT/c13_define.elf" "$ROOT_DIR/tests/c13_define.c"
OUT_LOG=$("$SIM" "$OUT/c13_define.elf" 2>&1 || true)
echo "$OUT_LOG" | grep -q "OK" || { echo "FAIL c13_define_sim" >&2; exit 1; }
echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL c13_define_sim" >&2; exit 1; }
echo "PASS c13_define_sim"
