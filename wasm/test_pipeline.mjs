import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

process.on("unhandledRejection", (err) => {
  console.error(err?.stack || err);
  process.exit(1);
});

process.on("uncaughtException", (err) => {
  console.error(err?.stack || err);
  process.exit(1);
});

if (process.env.MINA_WASM_NODE_TEST !== "1") {
  console.log("SKIP wasm pipeline: run the browser test in wasm/test_pipeline.html (set MINA_WASM_NODE_TEST=1 to force this Node test)."
  );
  process.exit(0);
}

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

let output = [];

try {
  console.error("[test] importing pipeline");
  const pipelinePath = resolve(__dirname, "pipeline.js");
  const pipeline = await import(`file://${pipelinePath}`);

  output = [];
  console.error("[test] initializing toolchain");
  const toolchain = await pipeline.initToolchain({
    onStdout: (line) => output.push(`[stdout] ${line}`),
    onStderr: (line) => output.push(`[stderr] ${line}`)
  });

  const source = "int main(){ putchar(79); putchar(75); putchar(10); return 0; }";

  console.error("[test] compile/assemble/run");
  toolchain.compileAssembleRun(source, { optimize: true });

  const outText = output.join("\n");
  if (!outText.includes("OK")) {
    console.error(outText);
    process.exit(1);
  }

  console.log("PASS wasm pipeline");
} catch (err) {
  if (output.length) {
    console.error(output.join("\n"));
  }
  console.error(err?.stack || err);
  process.exit(1);
}
