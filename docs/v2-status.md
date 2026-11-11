# v2 screen map and implementation status

Issue #4803 records the v2 direction without turning prototype coverage into a
support claim. The v2 contract is in [`design/v2`](../design/v2/), with the
canonical tokens in [`tokens.json`](../design/v2/tokens.json). The screen map is
the implementation order from issue #4784 and related follow-up work.

| Surface | v2 screen/state | Current status | Evidence or next step |
| --- | --- | --- | --- |
| GUI | Library ready | Implemented slice | Native `Main.qml` exposes Library browsing, selection, metadata and source-PCM visualization; native qualification remains open. |
| GUI | FFT mode | Implemented slice | Native visualization mode controls exist; verify against real source data on each platform. |
| GUI | Add-folder modal | Implemented slice | Native folder chooser, duplicate/error/progress/cancel states are present; run the QML contract tests. |
| GUI | DJ ready | Implemented slice | Native DJ page and recovery messaging exist; real output capture is not qualified. |
| GUI | DJ FFT mode | Implemented slice | Shared visualization mode exists in the DJ flow; native capture/FFT evidence remains required. |
| GUI | Output-picker modal | Implemented recovery slice | Native endpoint/display recovery controls exist; platform overlay and device qualification remain pending. |
| CLI | Live | Design-only | No v2 CLI screen implementation or committed screenshot; keep CLI output truthful and use the existing CLI contracts. |
| CLI | Library | Design-only | No v2 browse/detail TUI implementation or committed screenshot. |
| CLI | Command palette | Design-only | No v2 command-palette implementation or committed screenshot. |
| CLI | Settings | Design-only | No v2 settings surface implementation or committed screenshot. |

## Screenshot paths

The committed visual references currently live under
[`images/screenshots`](../images/screenshots/):

- [`library.png`](../images/screenshots/library.png)
- [`track-analysis.png`](../images/screenshots/track-analysis.png)
- [`dj-mode.png`](../images/screenshots/dj-mode.png)
- [`overlay-context.png`](../images/screenshots/overlay-context.png)

These are v1/design references, not native qualification captures. No v2
prototype PNGs are committed on `main`; do not cite an untracked local render as
implementation evidence. When v2 renders are added, use
`design/v2/output/<surface>/<screen>.png` and update this table in the same
change.

## Reproducible local checks

From the repository root:

```text
Push-Location design/v2; node check.mjs; Pop-Location
python tests/unit/check_overlay_design_system_contract.py
git diff --check
```

The token checker validates the JSON manifest against its typed QML and CLI
projections. These are portable checks; they do not qualify a native desktop,
audio endpoint, overlay, or screen-reader workflow.

The broader platform evidence and remaining gates are tracked in
[`platform-qualification-status.md`](platform-qualification-status.md), while
the foundation decision is in [`phase0-gate.md`](phase0-gate.md). Issue links:
[#4784](https://github.com/zfogg/beatglass/issues/4784) defines the token
contract; [#4803](https://github.com/zfogg/beatglass/issues/4803) tracks this
status documentation; issue #1 remains the product specification and exit-gate
authority.
