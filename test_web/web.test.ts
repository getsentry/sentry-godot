import { test, expect, Page } from "@playwright/test";
import fs from "node:fs";
import path from "node:path";

const COMPLETION_MARKER = ">>> Test run complete with code: ";
const PROJECT_DIR = path.resolve(__dirname, "../project");

// Discover test paths: suite tests first, then each isolated test file.
// Mirrors the pattern in scripts/run-android-tests.sh.
function discoverTestPaths(): string[] {
  const paths = ["res://test/suites/"];
  const isolatedDir = path.join(PROJECT_DIR, "test/isolated");
  if (fs.existsSync(isolatedDir)) {
    const files = fs
      .readdirSync(isolatedDir)
      .filter((f) => f.startsWith("test_") && f.endsWith(".gd"))
      .sort()
      .map((f) => `res://test/isolated/${f}`);
    paths.push(...files);
  }
  return paths;
}

// Start the Godot engine with the given test path and wait for completion.
// Returns the exit code from the test runner.
async function runGodotTests(page: Page, testPath: string): Promise<number> {
  const { promise: completionPromise, resolve: completionResolve } = Promise.withResolvers<number>();

  page.on("console", (msg) => {
    const text = msg.text();
    console.log(text);
    if (text.includes(COMPLETION_MARKER)) {
      const code = parseInt(text.split(COMPLETION_MARKER)[1], 10);
      completionResolve(code);
    }
  });

  page.on("pageerror", (error) => {
    console.log(`[PAGE ERROR] ${error.message}`);
  });

  await page.goto("/test.html");

  // Start the Godot engine with test arguments.
  // See: https://docs.godotengine.org/en/stable/tutorials/platform/web/customizing_html5_shell.html
  await page.evaluate((testArgs: string[]) => {
    const engine = new (window as any).Engine((window as any).GODOT_CONFIG);
    engine
      .startGame({ args: testArgs })
      .catch((err: Error) => {
        console.error("Engine start failed:", err.message);
      });
  }, ["--", "run-tests", testPath]);

  // Wait for the test completion marker in console output.
  // Note: onExit is unreliable — Godot's WASM shutdown may trap before it fires.
  return await completionPromise;
}

// Formatted output (matches scripts/run-android-tests.sh)
const success = (msg: string) => console.log(`\x1b[1;32m${msg}\x1b[0m`);
const error = (msg: string) => console.log(`\x1b[1;31m${msg}\x1b[0m`);

const TEST_PATHS = discoverTestPaths();

for (const testPath of TEST_PATHS) {
  test(`run-tests ${testPath}`, async ({ page }) => {
    console.log(`\n::group::Test log ${testPath}`);
    const exitCode = await runGodotTests(page, testPath);
    console.log("::endgroup::");
    if (exitCode === 0) {
      success(`✓ PASSED: ${testPath}`);
    } else {
      error(`✗ FAILED: ${testPath} (exit code: ${exitCode})`);
    }
    expect(exitCode).toBe(0);
  });
}
