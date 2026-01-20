import { initSimulator, loadBinary } from "./loader.js";
import { initToolchain } from "./pipeline.js";

const samples = [
  { name: "hello.elf", label: "hello" },
  { name: "c_prog_fib.elf", label: "fib" },
  { name: "c_prog_prime.elf", label: "prime" }
];

const binarySelect = document.getElementById("binary");
const runBtn = document.getElementById("run");
const clearBtn = document.getElementById("clear");
const output = document.getElementById("output");
const statusEl = document.getElementById("status");
const templateSelect = document.getElementById("template");
const buildBtn = document.getElementById("build");
const runBuiltBtn = document.getElementById("run-built");
const analyzeBtn = document.getElementById("analyze");
const sourceEl = document.getElementById("source");
const buildStatusEl = document.getElementById("build-status");

const templates = [
  {
    label: "hello (OK)",
    source: "#include \"clib.h\"\nint main(){ putchar(79); putchar(75); putchar(10); return 0; }"
  },
  {
    label: "fib (10 terms)",
    source: [
      "#include \"clib.h\"",
      "int main(){",
      "  int a=0;",
      "  int b=1;",
      "  int i=0;",
      "  while(i<10){",
      "    int c=a+b;",
      "    a=b;",
      "    b=c;",
      "    i=i+1;",
      "  }",
      "  putchar(79);",
      "  putchar(75);",
      "  putchar(10);",
      "  return 0;",
      "}"
    ].join("\n")
  }
];

function append(line) {
  output.textContent += line + "\n";
}

function appendStdout(line) {
  append(`[stdout] ${line}`);
}

function appendStderr(line) {
  append(`[stderr] ${line}`);
}

function setStatus(text) {
  statusEl.textContent = text;
}

function setBuildStatus(text) {
  buildStatusEl.textContent = text;
}

function fillSamples() {
  binarySelect.innerHTML = "";
  for (const s of samples) {
    const opt = document.createElement("option");
    opt.value = s.name;
    opt.textContent = `${s.label} (${s.name})`;
    binarySelect.appendChild(opt);
  }
}

function fillTemplates() {
  templateSelect.innerHTML = "";
  templates.forEach((t, idx) => {
    const opt = document.createElement("option");
    opt.value = String(idx);
    opt.textContent = t.label;
    templateSelect.appendChild(opt);
  });
  if (templates.length > 0) {
    templateSelect.value = "0";
    sourceEl.value = templates[0].source;
  }
}

async function main() {
  fillSamples();
  fillTemplates();
  runBtn.disabled = true;
  buildBtn.disabled = true;
  runBuiltBtn.disabled = true;
  analyzeBtn.disabled = true;
  setStatus("Loading WASM module…");
  setBuildStatus("Loading toolchain…");

  const binaries = [];
  for (const s of samples) {
    try {
      const data = await loadBinary(`./binaries/${s.name}`);
      binaries.push({ name: s.name, data });
    } catch (err) {
      append(`[loader] Failed to load ${s.name}: ${err}`);
    }
  }

  const sim = await initSimulator({
    binaries,
    onStdout: appendStdout,
    onStderr: appendStderr
  });

  const toolchain = await initToolchain({
    onStdout: appendStdout,
    onStderr: appendStderr
  });

  setStatus("Ready");
  runBtn.disabled = false;
  setBuildStatus("Ready");
  buildBtn.disabled = false;

  runBtn.addEventListener("click", () => {
    const name = binarySelect.value;
    append(`\n$ mina-sim ${name}`);
    try {
      setStatus(`Running ${name}…`);
      sim.runElf(name);
      setStatus("Ready");
    } catch (err) {
      append(`[error] ${err}`);
      setStatus("Failed");
    }
  });

  let lastElfPath = null;

  templateSelect.addEventListener("change", () => {
    const idx = Number(templateSelect.value);
    const tpl = templates[idx];
    if (tpl) sourceEl.value = tpl.source;
  });

  buildBtn.addEventListener("click", () => {
    try {
      setBuildStatus("Building…");
      append("\n$ minac --emit-asm input.c | mina-as");
      const src = sourceEl.value || "";
      const asmPath = toolchain.compileC(src, { optimize: true, quietAsm: true });
      toolchain.copyFile(toolchain.minac, asmPath, toolchain.minaAs, asmPath);
      const elfPath = toolchain.assemble(asmPath);
      toolchain.copyFile(toolchain.minaAs, elfPath, toolchain.sim, elfPath);
      lastElfPath = elfPath;
      setBuildStatus("Build OK");
      runBuiltBtn.disabled = false;
      analyzeBtn.disabled = false;
    } catch (err) {
      append(`[error] ${err}`);
      setBuildStatus("Build failed");
    }
  });

  runBuiltBtn.addEventListener("click", () => {
    if (!lastElfPath) return;
    try {
      setBuildStatus("Running…");
      append(`\n$ mina-sim ${lastElfPath}`);
      toolchain.runElf(lastElfPath);
      setBuildStatus("Ready");
    } catch (err) {
      append(`[error] ${err}`);
      setBuildStatus("Run failed");
    }
  });

  analyzeBtn.addEventListener("click", () => {
    if (!lastElfPath) return;
    try {
      setBuildStatus("Analyzing…");
      append(`\n$ mina-elf-info ${lastElfPath}`);
      toolchain.analyzeElf(lastElfPath, { stats: true });
      setBuildStatus("Ready");
    } catch (err) {
      append(`[error] ${err}`);
      setBuildStatus("Analyze failed");
    }
  });

  clearBtn.addEventListener("click", () => {
    output.textContent = "";
  });
}

main().catch((err) => {
  append(`[fatal] ${err}`);
  setStatus("Failed");
});
