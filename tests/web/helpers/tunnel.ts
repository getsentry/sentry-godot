import type { Page } from "@playwright/test";

// Matches the capture_message() text from test_web_tunnel.gd in the envelope body.
const EVENT_MARKER = Buffer.from("web tunnel regression");

export async function installTunnelProbe(page: Page): Promise<void> {
  await page.route("**/*", async (route) => {
    const request = route.request();
    if (request.method() !== "POST") {
      return route.continue();
    }
    if (request.postDataBuffer()?.includes(EVENT_MARKER)) {
      await page.evaluate((url) => {
        const destination = new URL(url);
        (window as any).__tunnelUploadUrl = destination.origin === location.origin ? destination.pathname : url;
      }, request.url());
    }
    await route.fulfill({ status: 200, body: "{}", headers: { "Access-Control-Allow-Origin": "*" } });
  });
}
