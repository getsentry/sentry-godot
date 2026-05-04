import { defineConfig } from "@playwright/test";

const WEB_EXPORT_DIR = process.env.WEB_EXPORT_DIR || "../exports/web";

export default defineConfig({
  testDir: ".",
  testMatch: "*.test.ts",
  timeout: 120_000,
  workers: 1,
  use: {
    baseURL: "http://localhost:8521",
  },
  projects: [
    {
      name: "chromium",
      use: {
        browserName: "chromium",
        launchOptions: {
          args: ["--use-gl=swiftshader"],
        },
      },
    },
  ],
  webServer: {
    command: `npx tsx serve.ts`,
    port: 8521,
    reuseExistingServer: !process.env.CI,
    env: {
      WEB_EXPORT_DIR,
    },
  },
});
