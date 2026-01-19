import { initToolchain } from "./pipeline.js";

const statusEl = document.getElementById("status");
const outputEl = document.getElementById("output");

const output = [];
const source = "int main(){ putchar(79); putchar(75); putchar(10); return 0; }";

function log(line) {
  output.push(line);
  outputEl.textContent = output.join("\n");
}

try {
  const toolchain = await initToolchain({
    onStdout: (line) => log(`[stdout] ${line}`),
    onStderr: (line) => log(`[stderr] ${line}`)
  });

  toolchain.compileAssembleRun(source, { optimize: true });

  const outText = output.join("\n");
  if (outText.includes("OK")) {
    statusEl.textContent = "PASS";
    statusEl.className = "ok";
  } else {
    statusEl.textContent = "FAIL (missing OK)";
    statusEl.className = "fail";
  }
} catch (err) {
  log(err?.stack || String(err));
  statusEl.textContent = "FAIL (exception)";
  statusEl.className = "fail";
}
