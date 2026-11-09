# Beatglass v1 prototype

This is a static, illustrative design specification, not the native application. Sample music, analysis and DJ application content are fictional. Buttons and controls are visual examples; they do not perform audio capture, playback, theme persistence, or file operations. Native implementation and issue #1 qualification remain open.

## View and render

From this directory, run `npm ci`, then `npm run render` to write 60 PNGs to the ignored `output/` directory. `npm run render -- path/to/directory` writes there instead. Chrome is expected at `C:/Program Files/Google/Chrome/Application/chrome.exe`; set `CHROME_PATH` to a Chromium-compatible executable elsewhere. Open `index.html?screen=library&theme=light` directly in a browser; change the query parameters to any screen and theme below. `npm run check` checks content and desktop overflow, not functional interaction or accessibility compliance.

Screens: `library`, `library-empty`, `library-filter-empty`, `library-storage-recovery`, `track`, `track-loading`, `track-error`, `track-queued`, `jobs`, `jobs-empty`, `dj`, `dj-recovery`, `dj-permission`, `dj-paused`, `overlay`, `overlay-context`, `settings`, `recovery`. Themes: `light`, `dark`, `contrast`, `purple`, `blue`, `yellow`. `snapshots/` contains the tracked 1440×960 reference renders. The overlay-in-context image shows an illustrative DJ application behind the floating panel; it is not a screenshot of a real DJ application.

## Files

- [PRODUCT.md](PRODUCT.md): product facts and constraints.
- [DESIGN.md](DESIGN.md): visual language, exact construction and component contracts.
- [STATES.md](STATES.md): screen coverage, transitions and implementation acceptance criteria.
- [REVIEW.md](REVIEW.md): critique, audit, validation and limitations.
- `style.css` and `app.js`: executable source of truth for prototype geometry, tokens, sample data and rendered composition. Use the CSS values when translating to native Qt logical pixels; do not embed this web prototype in the product.
