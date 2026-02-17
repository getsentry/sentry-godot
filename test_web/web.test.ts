import { test, expect } from "@playwright/test";

const COMPLETION_MARKER = ">>> Test run complete with code: ";

test("suite tests pass in web export", async ({ page }) => {
	const output: string[] = [];
	let completionResolve: (code: number) => void;
	const completionPromise = new Promise<number>((resolve) => {
		completionResolve = resolve;
	});

	// Capture all console output; detect test completion marker
	page.on("console", (msg) => {
		const text = msg.text();
		output.push(text);
		if (text.includes(COMPLETION_MARKER)) {
			const code = parseInt(text.split(COMPLETION_MARKER)[1], 10);
			completionResolve(code);
		}
	});

	// Capture unhandled page errors for debugging
	page.on("pageerror", (error) => {
		output.push(`[PAGE ERROR] ${error.message}`);
	});

	// Navigate to test page (server generates it from discovered export files)
	await page.goto("/test.html");

	// Start the Godot engine with test arguments
	// Pattern follows: https://docs.godotengine.org/en/stable/tutorials/platform/web/customizing_html5_shell.html
	await page.evaluate(() => {
		const engine = new (window as any).Engine((window as any).GODOT_CONFIG);
		engine
			.startGame({
				args: ["--", "run-tests", "res://test/suites/"],
				onPrint: (...args: any[]) => {
					console.log(...args);
				},
				onPrintError: (...args: any[]) => {
					console.error(...args);
				},
			})
			.catch((err: Error) => {
				console.error("Engine start failed:", err.message);
			});
	});

	// Wait for the test completion marker in console output.
	// Note: onExit is unreliable — Godot's WASM shutdown may trap before it fires.
	const exitCode = await completionPromise;

	// Collect output for debugging
	const fullOutput = output.join("\n");
	console.log(fullOutput);

	// Assert
	expect(exitCode).toBe(0);
});
