# v2 token contract

`tokens.json` is the canonical, versioned vocabulary for the v2 GUI and CLI. A
token is semantic: consumers must use `color.energy` for energy and not invent a
second cyan. Numeric dimensions are device-independent pixels in QML and CSS
pixels in the prototype. Durations are milliseconds. The manifest is deliberately
small; a new token needs a design-system rationale and a corresponding test.

## Consumers

- The web prototype maps the manifest to CSS custom properties in `style.css`.
- QML imports the singleton `V2Tokens` from `src/desktop/qml/V2Tokens.qml` and
  uses its camelCase properties. QML owns scaling; do not bake platform DPI into
  the manifest.
- CLI/TUI code includes `src/cli/v2_tokens.hpp`. These are presentation constants
  only; terminal capability detection chooses whether to emit ANSI colors. A
  non-color terminal must retain the semantic labels and state text.

The JSON remains the reviewable source of truth. The QML and C++ files are small
typed projections so native consumers do not parse JSON at runtime. Run
`node design/v2/check.mjs` after changing any projection. The checker fails on
missing required tokens, invalid colors, or projection drift.

Color is never a status-only signal: pair semantic colors with labels/state text.
The manifest does not authorize writes to user audio files or metadata.
