# Sentry Bridge for Web integration

A TypeScript bridge connecting Godot web exports to Sentry JavaScript SDK for error tracking.

## Project Structure

```
bridge/
├── src/
│   └── sentry-bridge.ts      # Core bridge implementation
├── dist/                     # Compiled output (auto-generated)
├── package.json
├── tsconfig.json
└── webpack.config.js
```

## Commands

- `npm run build` - Build production bundle
- `npm run build:deploy` - Build and copy to `project/addons/sentry/web/`
- `npm run clean` - Clean build artifacts
- `npm test` - Run tests

## API

The bridge exposes `window.SentryBridge`. Example usage:

```typescript
SentryBridge.init(beforeSendCallback, beforeSendLogCallback, dsn, debug, release, dist, environment, sampleRate, maxBreadcrumbs, enableLogs)
SentryBridge.setTag(key, value)
SentryBridge.setUser(id, username, email, ip)
SentryBridge.captureMessage(message, level)
SentryBridge.captureEvent(event)
```

See `src/sentry-bridge.ts` for all available methods.

## Build Output

Output is written to `dist/`. Use `npm run build:deploy` to copy to `project/addons/sentry/web/`.

- `sentry-bundle.js` - Minified bundle with Sentry SDK included
- `sentry-bundle.js.map` - Source map for debugging
