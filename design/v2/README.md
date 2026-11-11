# Beatglass v2 design prototypes

This is a Playwright-rendered visual design project for the next GUI and CLI direction. It intentionally focuses the huge v1 state gallery into a smaller, implementable set of high-value surfaces.

## Render

```powershell
npm install
npm run render -- output
```

The output argument defaults to `output/` and produces:

- `output/gui/` — Library, FFT, folder picker, DJ, output picker
- `output/cli/` — live, Library, command palette, Settings

The renders use synthetic, clearly labeled data. They are design evidence, not a claim that every pictured popup is already wired into the native app. The colorful waveform and FFT are intentionally a stronger target than v1's blocky visualization; native implementation must map actual PCM/FFT data into the same visual contract.

See [TOKENS.md](TOKENS.md) and [tokens.json](tokens.json) for the shared v2 token manifest and the QML/CLI consumption contract. Validate projections with `npm run check`.
