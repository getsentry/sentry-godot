import http from "node:http";
import fs from "node:fs";
import path from "node:path";
import escapeHtml from "escape-html";

const PORT = parseInt(process.env.PORT || "8521", 10);
const EXPORT_DIR = path.resolve(process.env.WEB_EXPORT_DIR || "../exports/web");

const MIME_TYPES: Record<string, string> = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript",
  ".mjs": "application/javascript",
  ".wasm": "application/wasm",
  ".pck": "application/octet-stream",
  ".png": "image/png",
  ".ico": "image/x-icon",
  ".svg": "image/svg+xml",
  ".css": "text/css",
  ".map": "application/json",
};

interface ExportInfo {
  executable: string;
  engineJs: string;
  gdextensionLibs: string[];
  hasThreads: boolean;
  hasSentryBundle: boolean;
}

function discoverExportFiles(): ExportInfo {
  const SAFE_FILENAME = /^[\w.-]+$/;
  const files = fs.readdirSync(EXPORT_DIR).filter((f) => SAFE_FILENAME.test(f));

  // Find .pck to derive executable name
  const pckFile = files.find((f) => f.endsWith(".pck"));
  if (!pckFile) {
    throw new Error(`No .pck file found in ${EXPORT_DIR}`);
  }
  const executable = pckFile.replace(".pck", "");

  // Engine JS has the same base name as the executable
  const engineJs = `${executable}.js`;
  if (!files.includes(engineJs)) {
    throw new Error(`Engine JS not found: ${engineJs}`);
  }

  // GDExtension libs: .wasm files that aren't the main wasm or side wasm
  const gdextensionLibs = files.filter(
    (f) => f.endsWith(".wasm") && f !== `${executable}.wasm` && f !== `${executable}.side.wasm`,
  );

  // Threads build has a .side.wasm file
  const hasThreads = files.includes(`${executable}.side.wasm`);

  const hasSentryBundle = files.includes("sentry-bundle.js");

  return { executable, engineJs, gdextensionLibs, hasThreads, hasSentryBundle };
}

function generateTestHtml(info: ExportInfo): string {
  const godotConfig = {
    args: [],
    canvasResizePolicy: 0,
    executable: escapeHtml(info.executable),
    experimentalVK: false,
    focusCanvas: false,
    gdextensionLibs: info.gdextensionLibs.map(escapeHtml),
    ensureCrossOriginIsolationHeaders: info.hasThreads,
  };
  return `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>Godot Web Tests</title></head>
<body>
<canvas id="canvas" width="800" height="600"></canvas>
${info.hasSentryBundle ? '<script src="sentry-bundle.js"></script>' : ""}
<script src="${escapeHtml(info.engineJs)}"></script>
<script>window.GODOT_CONFIG = ${JSON.stringify(godotConfig)};</script>
</body>
</html>`;
}

const info = discoverExportFiles();
const testHtml = generateTestHtml(info);

console.log(`Export dir: ${EXPORT_DIR}`);
console.log(`Executable: ${info.executable}, threads: ${info.hasThreads}`);
console.log(`GDExtension libs: ${info.gdextensionLibs.join(", ")}`);

// Cross-origin isolation headers (required for threads, harmless for nothreads)
const COI_HEADERS: Record<string, string> = {
  "Cross-Origin-Opener-Policy": "same-origin",
  "Cross-Origin-Embedder-Policy": "require-corp",
};

const server = http.createServer((req, res) => {
  const url = new URL(req.url || "/", `http://localhost:${PORT}`);

  if (url.pathname === "/" || url.pathname === "/test.html") {
    res.writeHead(200, {
      ...COI_HEADERS,
      "Content-Type": "text/html; charset=utf-8",
    });
    res.end(testHtml);
    return;
  }

  // Prevent directory traversal.
  const filePath = path.resolve(EXPORT_DIR, url.pathname.slice(1));
  if (!filePath.startsWith(EXPORT_DIR + path.sep)) {
    res.writeHead(403);
    res.end("Forbidden");
    return;
  }

  let stat: fs.Stats;
  try {
    stat = fs.statSync(filePath);
  } catch {
    res.writeHead(404);
    res.end("Not Found");
    return;
  }
  if (!stat.isFile()) {
    res.writeHead(404);
    res.end("Not Found");
    return;
  }

  const ext = path.extname(filePath).toLowerCase();
  const contentType = MIME_TYPES[ext] || "application/octet-stream";

  res.writeHead(200, {
    ...COI_HEADERS,
    "Content-Type": contentType,
    "Content-Length": stat.size.toString(),
    "Cross-Origin-Resource-Policy": "same-origin",
  });

  fs.createReadStream(filePath).pipe(res);
});

server.listen(PORT, () => {
  console.log(`Test server listening on http://localhost:${PORT}`);
});
