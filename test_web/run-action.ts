/**
 * Runs a single CLI action in the Godot web export via headless Chromium.
 *
 * Usage:
 *   npx tsx run-action.ts <export-dir> [-- <godot-args...>]
 *
 * Prints captured console lines to stdout (one per line).
 * Exits with the detected app exit code.
 */

import { spawn } from "node:child_process";
import path from "node:path";
import { chromium } from "@playwright/test";

const args = process.argv.slice(2);
if (args.length < 1) {
	console.error(
		"Usage: npx tsx run-action.ts <export-dir> [-- <godot-args...>]",
	);
	process.exit(2);
}

const EXPORT_DIR = path.resolve(args[0]);
const godotArgs = args.slice(1);

const PORT = 8521;
const EXIT_MARKER = ">>> App exit with code: ";

function startServer() {
	return spawn(
		process.execPath,
		[...process.execArgv, path.join(import.meta.dirname, "serve.ts")],
		{
			env: { ...process.env, WEB_EXPORT_DIR: EXPORT_DIR, PORT: String(PORT) },
			stdio: ["ignore", "pipe", "inherit"],
		},
	);
}

async function waitForServer(url: string, timeoutMs = 15000): Promise<void> {
	const start = Date.now();
	while (Date.now() - start < timeoutMs) {
		try {
			await fetch(url);
			return;
		} catch {
			await new Promise((r) => setTimeout(r, 200));
		}
	}
	throw new Error(`Server not ready after ${timeoutMs}ms`);
}

async function main() {
	const serverProc = startServer();

	const baseUrl = `http://127.0.0.1:${PORT}`;
	await waitForServer(baseUrl);

	const browser = await chromium.launch({
		headless: true,
		args: ["--use-gl=swiftshader", "--no-sandbox"],
	});

	try {
		const page = await (await browser.newContext()).newPage();
		const { promise: completionPromise, resolve: completionResolve } =
			Promise.withResolvers<number>();

		page.on("console", (msg) => {
			const text = msg.text();
			console.log(text);
			if (text.includes(EXIT_MARKER)) {
				completionResolve(parseInt(text.split(EXIT_MARKER)[1], 10));
			}
		});

		page.on("pageerror", (error) => {
			console.log(`[PAGE ERROR] ${error.message}`);
		});

		await page.goto(`${baseUrl}/test.html`);

		await page.evaluate((engineArgs: string[]) => {
			const engine = new (window as any).Engine((window as any).GODOT_CONFIG);
			engine.startGame({ args: engineArgs }).catch((err: Error) => {
				console.error("Engine start failed:", err.message);
			});
		}, godotArgs);

		const exitCode = await completionPromise;
		process.exitCode = exitCode;
	} finally {
		await browser.close();
		serverProc.kill();
	}
}

main().catch((err) => {
	console.error(err);
	process.exit(2);
});
