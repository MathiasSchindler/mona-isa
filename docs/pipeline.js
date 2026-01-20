import createMinac from "./minac.js";
import createMinaAs from "./mina-as.js";
import createSim from "./mina-sim.js";
import createElfInfo from "./mina-elf-info.js";

const isNode = typeof window === "undefined";

async function loadWasmBinary(name) {
  if (!isNode) return null;
  const { readFile } = await import("node:fs/promises");
  const { fileURLToPath } = await import("node:url");
  const { dirname, resolve } = await import("node:path");
  const __filename = fileURLToPath(import.meta.url);
  const __dirname = dirname(__filename);
  const wasmPath = resolve(__dirname, name);
  const data = await readFile(wasmPath);
  return new Uint8Array(data);
}

async function loadTextFile(name) {
  if (isNode) {
    const { readFile } = await import("node:fs/promises");
    const { fileURLToPath } = await import("node:url");
    const { dirname, resolve } = await import("node:path");
    const __filename = fileURLToPath(import.meta.url);
    const __dirname = dirname(__filename);
    const path = resolve(__dirname, name);
    return readFile(path, "utf8");
  }
  const url = new URL(name, import.meta.url).href;
  const res = await fetch(url);
  if (!res.ok) throw new Error(`Failed to load ${name}`);
  return res.text();
}

function locateFileFactory(baseUrl) {
  return (path) => new URL(path, baseUrl).href;
}

export async function initToolchain({ onStdout = () => {}, onStderr = () => {} } = {}) {
  const baseUrl = import.meta.url;
  const minacStdout = { current: null, suppress: false };
  const minacPrint = (line) => {
    if (minacStdout.current) minacStdout.current.push(line);
    if (!minacStdout.suppress) onStdout(line);
  };
  const [minacWasm, minaAsWasm, simWasm, elfInfoWasm] = await Promise.all([
    loadWasmBinary("minac.wasm"),
    loadWasmBinary("mina-as.wasm"),
    loadWasmBinary("mina-sim.wasm"),
    loadWasmBinary("mina-elf-info.wasm")
  ]);

  const [minac, minaAs, sim, elfInfo] = await Promise.all([
    createMinac({
      noInitialRun: true,
      noExitRuntime: true,
      print: minacPrint,
      printErr: onStderr,
      wasmBinary: minacWasm || undefined,
      locateFile: locateFileFactory(baseUrl)
    }),
    createMinaAs({
      noInitialRun: true,
      noExitRuntime: true,
      print: onStdout,
      printErr: onStderr,
      wasmBinary: minaAsWasm || undefined,
      locateFile: locateFileFactory(baseUrl)
    }),
    createSim({
      noInitialRun: true,
      noExitRuntime: true,
      print: onStdout,
      printErr: onStderr,
      wasmBinary: simWasm || undefined,
      locateFile: locateFileFactory(baseUrl)
    }),
    createElfInfo({
      noInitialRun: true,
      noExitRuntime: true,
      print: onStdout,
      printErr: onStderr,
      wasmBinary: elfInfoWasm || undefined,
      locateFile: locateFileFactory(baseUrl)
    })
  ]);

  for (const mod of [minac, minaAs, sim, elfInfo]) {
    if (!mod.FS.analyzePath("/data").exists) {
      mod.FS.mkdir("/data");
    }
  }

  for (const mod of [minac]) {
    if (!mod.FS.analyzePath("/clib").exists) mod.FS.mkdir("/clib");
    if (!mod.FS.analyzePath("/clib/include").exists) mod.FS.mkdir("/clib/include");
    if (!mod.FS.analyzePath("/clib/src").exists) mod.FS.mkdir("/clib/src");
  }

  const [clibHeader, clibSource] = await Promise.all([
    loadTextFile("clib.h"),
    loadTextFile("clib.c")
  ]);
  writeFile(minac, "/clib/include/clib.h", new TextEncoder().encode(clibHeader));
  writeFile(minac, "/clib/src/clib.c", new TextEncoder().encode(clibSource));

  function writeFile(mod, path, data) {
    if (mod.FS.analyzePath(path).exists) mod.FS.unlink(path);
    mod.FS.writeFile(path, data);
  }

  function copyFile(fromMod, fromPath, toMod, toPath) {
    const data = fromMod.FS.readFile(fromPath);
    writeFile(toMod, toPath, data);
  }

  let cachedClibAsm = null;

  function compileClibAsm() {
    if (cachedClibAsm) return cachedClibAsm;
    const args = ["--emit-asm", "--no-start", "/clib/src/clib.c"];
    const captured = [];
    minacStdout.current = captured;
    minacStdout.suppress = true;
    try {
      minac.callMain(args);
    } finally {
      minacStdout.current = null;
      minacStdout.suppress = false;
    }
    cachedClibAsm = `${captured.join("\n")}\n`;
    return cachedClibAsm;
  }

  function compileC(sourceText, opts = {}) {
    const srcPath = "/data/input.c";
    const asmPath = "/data/out.s";
    writeFile(minac, srcPath, new TextEncoder().encode(sourceText));
    const args = ["--emit-asm"];
    const withClib = opts.withClib !== false;
    if (withClib) args.push("--prefer-libc");
    if (opts.optimize) args.push("-O");
    args.push(srcPath);
    const captured = [];
    minacStdout.current = captured;
    minacStdout.suppress = !!opts.quietAsm;
    try {
      minac.callMain(args);
    } finally {
      minacStdout.current = null;
      minacStdout.suppress = false;
    }
    let asmText = `${captured.join("\n")}\n`;
    if (withClib) {
      const clibAsm = compileClibAsm();
      asmText = `${clibAsm}\n${asmText}`;
    }
    writeFile(minac, asmPath, new TextEncoder().encode(asmText));
    return asmPath;
  }

  function assemble(asmPath, opts = {}) {
    const elfPath = "/data/out.elf";
    const args = [asmPath, "-o", elfPath];
    if (opts.bin) args.unshift("--bin");
    minaAs.callMain(args);
    return elfPath;
  }

  function runElf(elfPath, simArgs = []) {
    const args = [...simArgs, elfPath];
    sim.callMain(args);
  }

  function analyzeElf(elfPath, opts = {}) {
    const targetPath = "/data/analyze.elf";
    copyFile(sim, elfPath, elfInfo, targetPath);
    const args = [];
    if (opts.onlyText) args.push("--only-text");
    if (opts.stats) args.push("--stats");
    if (opts.json) args.push("--json");
    if (opts.stopAtEbreak) args.push("--stop-at-ebreak");
    args.push(targetPath);
    elfInfo.callMain(args);
  }

  function compileAssembleRun(sourceText, opts = {}) {
    const asmPath = compileC(sourceText, opts);
    copyFile(minac, asmPath, minaAs, asmPath);
    const elfPath = assemble(asmPath, opts);
    copyFile(minaAs, elfPath, sim, elfPath);
    runElf(elfPath, opts.simArgs || []);
  }

  return { minac, minaAs, sim, elfInfo, compileC, assemble, runElf, analyzeElf, compileAssembleRun, copyFile, writeFile };
}
