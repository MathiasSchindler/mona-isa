# MINA Simulator — WASM Build (Step 2)

This directory contains the WASM build setup for the MINA simulator.

## Build

```sh
make
```

Output:
- mina-sim.js
- mina-sim.wasm
- mina-as.js
- mina-as.wasm
- minac.js
- minac.wasm

## Notes
- Uses Emscripten `emcc` with:
  - `MODULARIZE=1`, `EXPORT_ES6=1`
  - `ALLOW_MEMORY_GROWTH=1`
  - `EXIT_RUNTIME=1`
  - `ENVIRONMENT=web,node`
  - `EXPORTED_RUNTIME_METHODS=["FS","callMain"]`

## Inputs
- Sources: `../simulator/src/main.c`, `cpu.c`, `mem.c`
- Includes: `../simulator/src`

## W5: mina-as (assembler/linker) WASM build

Build target is included in the same Makefile and emits:
- `mina-as.js`
- `mina-as.wasm`

You can load it similarly to the simulator and call `callMain` with input `.s` and output `.elf` paths in the Emscripten FS.

## W6: minac (C compiler) WASM build

Build target is included in the same Makefile and emits:
- `minac.js`
- `minac.wasm`

Notes:
- In the browser, `minac` should be used with `--emit-asm` or `--emit-ir`. Linking via `mina-as` is handled in W7 (JS pipeline), not via `system()` inside `minac`.
- File I/O uses the Emscripten FS; provide inputs via `/data`.

## W7: In‑browser compile pipeline

Use [wasm/pipeline.js](wasm/pipeline.js) to compile C → assemble → run in the browser.

Example:

```js
import { initToolchain } from "./pipeline.js";

const toolchain = await initToolchain({
  onStdout: (line) => console.log(line),
  onStderr: (line) => console.error(line)
});

const source = `int main(){ putchar(79); putchar(75); return 0; }`;
toolchain.compileAssembleRun(source, { optimize: true });
```

Test:

Open [wasm/index.html](wasm/index.html) via a local static server.

## Step 3: Virtual FS loader

Use [wasm/loader.js](wasm/loader.js) to load `.elf` files into the Emscripten FS and run the simulator:

```js
import { initSimulator, loadBinary } from "./loader.js";

const hello = await loadBinary("./binaries/hello.elf");
const sim = await initSimulator({
  binaries: [{ name: "hello.elf", data: hello }],
  onStdout: (line) => console.log(line),
  onStderr: (line) => console.error(line)
});

sim.runElf("hello.elf");
```

## Step 4: Browser UI

Open [wasm/index.html](wasm/index.html) via a local static server. The UI loads sample binaries from `wasm/binaries/` and runs the simulator in‑browser.

Files:
- [wasm/index.html](wasm/index.html)
- [wasm/app.js](wasm/app.js)

Sample list is defined in `app.js` and expects these files under `wasm/binaries/`:
- `hello.elf`
- `c_prog_fib.elf`
- `c_prog_prime.elf`

Sync helper:

```sh
./sync_binaries.sh
```
## W8: In‑browser editor

Open [wasm/index.html](wasm/index.html). The left pane includes a C editor, Build/Run, and ELF Info. The right pane shows output.

## W9: C library in the browser

The in‑browser toolchain bundles a small `clib` automatically. When users compile C code in the UI, `clib` is linked by default and the compiler prefers library calls.

Supported subset includes:
- `putchar`, `puts`, `exit`
- `strlen`, `memcpy`, `memset`
- `printf`/`vprintf` (limited formats)
- `isdigit`, `isalpha`, `isspace`
- `atoi`, `strtol`
- `write` (fd=1/2), `read` (stub)
- `malloc`/`free`/`calloc`/`realloc` (minimal)

## W10: Demo packaging

The demo is static. Serve the `wasm/` folder over HTTP (no backend required).

Entry point:
- [wasm/index.html](wasm/index.html) — main UI
