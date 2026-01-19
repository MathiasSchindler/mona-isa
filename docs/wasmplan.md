# WASM/Emscripten Plan — MINA Simulator in Browser

Goal: Compile the MINA simulator to WebAssembly so it runs in a browser with a curated set of binaries, no local install required.

## Scope
- Build the existing simulator (C/C++) with Emscripten.
- Provide a small browser UI that lets users select and run sample `.elf` binaries.
- Capture simulator output and show it in the page.

## Constraints
- No native file system access in browser; use in‑memory virtual FS.
- Keep the simulator core unchanged where possible.
- Use a minimal web UI (no heavy frameworks).

## Implementation Steps
1. **Inventory simulator build**
   - Identify simulator sources and entry point.
   - Verify it runs from CLI with an ELF path and prints output to stdout.
   - **Status:** Done. See `wasm/step1_inventory.md`.

2. **Create Emscripten build**
   - Add a `wasm/Makefile` or `wasm/build.sh` to compile the simulator with `emcc`.
   - Export a C API entry point for `run_elf(const uint8_t *data, size_t len)` if needed, or keep `main()` and feed ELF via Emscripten FS.
   - Decide on `MODULARIZE=1`, `EXPORT_ES6=1`, and `ALLOW_MEMORY_GROWTH=1` for browser compatibility.
   - **Status:** Done. See `wasm/Makefile` and `wasm/README.md`.

3. **Virtual FS loader**
   - Use `FS.createDataFile` or `FS.writeFile` to place sample `.elf` files in `/data`.
   - Call simulator main or exported function with `/data/<name>.elf`.
   - **Status:** Done. See `wasm/loader.js` and `wasm/README.md`.

4. **Browser UI**
   - Simple `index.html` + `wasm.js` loader.
   - Dropdown for sample binaries.
   - Run button, output area, clear button.
   - **Status:** Done. See `wasm/index.html` and `wasm/app.js`.

5. **Sample binaries**
   - Place a small curated set under `wasm/binaries/` (e.g., hello, fib, prime, struct/union).
   - Add a script to copy from `out/elf/` (or regenerate) into `wasm/binaries/`.

6. **Output capture**
   - Redirect stdout/stderr from the WASM module into the UI.
   - Display simulator status and exit code.

7. **Docs and usage**
   - Write `wasm/README.md` with build and run instructions.
   - Provide local dev server instructions (e.g., `python -m http.server`).

## Milestones
- **W1:** Emscripten build of simulator succeeds.
- **W2:** Simulator runs a single embedded ELF in browser.
- **W3:** UI with selectable binaries and output display.
- **W4:** Documented build pipeline and demo instructions.

## Extended Milestones — Toolchain in Browser (C Compiler + Assembler/Linker)

### W5 — WASM Build: Assembler/Linker
**Scope**
- Compile `mina-as` to WebAssembly.

**Tasks**
- Add `wasm/Makefile` targets for `mina-as` with Emscripten.
- Expose `callMain` and FS access.
- Verify assembling a `.s` into `.elf` inside browser FS.

**Outputs**
- `wasm/mina-as.js`, `wasm/mina-as.wasm`

**Status:** Done. See `wasm/Makefile` and `wasm/README.md`.

### W6 — WASM Build: C Compiler
**Scope**
- Compile `compiler/minac` to WebAssembly.

**Tasks**
- Add `wasm/Makefile` targets for `minac` with Emscripten.
- Ensure preprocessor works with virtual FS.
- Verify `minac` emits `.s` or `.elf` in FS.

**Outputs**
- `wasm/minac.js`, `wasm/minac.wasm`

**Status:** Done. See `wasm/Makefile` and `wasm/README.md`.

### W7 — In‑Browser Compile Pipeline
**Scope**
- Wire `minac` → `mina-as` → `mina-sim` in browser FS.

**Tasks**
- Implement JS pipeline helpers for compile/assemble/run.
- Add error capture for each stage.

**Status:** Done. See `wasm/pipeline.js` and `wasm/README.md`.

### W8 — Web UI: Code Editor + Build/Run
**Scope**
- Add code editor textarea and Build/Run buttons.

**Tasks**
- Provide editable C source area and output pane.
- Display compiler/assembler diagnostics.
- Optionally add a sample selector to load templates.

**Status:** Done. See `wasm/index.html` and `wasm/app.js`.

### W9 — C Library / Runtime Constraints
**Scope**
- Define supported C library subset in browser.

**Tasks**
- Document supported functions (e.g., `putchar`, `puts`, `exit`).
- Add a minimal runtime if needed.

### W10 — Docs + Demo Packaging
**Scope**
- Document full in‑browser toolchain.

**Tasks**
- Update `wasm/README.md` with build/run instructions.
- Provide a simple demo page with examples.

**Status:** Done. See `wasm/README.md` and `wasm/index.html`.

## Risks / Open Questions
- Simulator might use unsupported syscalls or file APIs; may need minor adaptation.
- If simulator is C++ or depends on non‑portable features, Emscripten flags may need tuning.

## Deliverables
- `wasm/wasmplan.md` (this plan)
- `wasm/README.md`
- Build script or Makefile for wasm
- Minimal HTML/JS demo
