#!/bin/sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
WASM_DIR="$ROOT_DIR/wasm"
BIN_DIR="$WASM_DIR/binaries"

mkdir -p "$BIN_DIR"

cp "$ROOT_DIR/simulator/hello.elf" "$BIN_DIR/hello.elf"
cp "$ROOT_DIR/out/elf/c_prog_fib.elf" "$BIN_DIR/c_prog_fib.elf"
cp "$ROOT_DIR/out/elf/c_prog_prime.elf" "$BIN_DIR/c_prog_prime.elf"

echo "Synced binaries to $BIN_DIR"
