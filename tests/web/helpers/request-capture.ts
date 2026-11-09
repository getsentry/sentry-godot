import type { Page } from "@playwright/test";

export async function installRequestCapture(page: Page): Promise<void> {
  await page.addInitScript(() => {
    (window as any).__sentryTestRequests = [];
  });
  await page.route("https://sentry-test.invalid/**", async (route) => {
    const request = route.request();
    if (request.method() === "POST") {
      await page.evaluate((captured) => {
        (window as any).__sentryTestRequests.push(captured);
      }, {
        headers: request.headers(),
        body: request.postDataBuffer()?.toString("base64") ?? "",
      });
    }
    await route.fulfill({
      status: 200,
      body: "{}",
      headers: {
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "POST, OPTIONS",
        "Access-Control-Allow-Headers": "*",
      },
    });
  });
}
