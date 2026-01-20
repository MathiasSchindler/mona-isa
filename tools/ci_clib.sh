#!/bin/sh
set -eu

# CI helper for clib: run compiler tests (includes clib suite).
ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR/compiler"
./tests/run.sh
