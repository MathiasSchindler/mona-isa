import createModule from "./mina-sim.js";

export async function loadBinary(url) {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`Failed to fetch ${url}: ${res.status}`);
    const buf = await res.arrayBuffer();
    return new Uint8Array(buf);
}

export async function initSimulator({
    binaries = [],
    onStdout = (line) => console.log(line),
    onStderr = (line) => console.error(line)
} = {}) {
    const Module = await createModule({
        noInitialRun: true,
        print: onStdout,
        printErr: onStderr
    });

    if (!Module.FS.analyzePath("/data").exists) {
        Module.FS.mkdir("/data");
    }

    for (const bin of binaries) {
        if (!bin || !bin.name || !bin.data) continue;
        const path = `/data/${bin.name}`;
        if (Module.FS.analyzePath(path).exists) {
            Module.FS.unlink(path);
        }
        Module.FS.writeFile(path, bin.data);
    }

    function runElf(name, args = []) {
        const path = `/data/${name}`;
        if (!Module.FS.analyzePath(path).exists) {
            throw new Error(`Missing ELF in /data: ${name}`);
        }
        const argv = [...args, path];
        return Module.callMain(argv);
    }

    return { Module, runElf };
}
