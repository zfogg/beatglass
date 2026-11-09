from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = Path("docs/overlay-recovery-workflow.md").read_text(encoding="utf-8")
rendering = Path("docs/overlay-recovery-workflow.svg").read_text(encoding="utf-8")

for marker in (
    'objectName: "overlayFallbackState"',
    'objectName: "overlayFallbackReviewAction"',
    'objectName: "overlaySettingsSummary"',
    'objectName: "restoreOverlay"',
    'objectName: "retryOverlay"',
):
    assert marker in qml, marker
for marker in ("Review recovery", "Restore overlay", "Retry overlay", "Screenshot qualification"):
    assert marker in docs, marker
assert "Overlay recovery workflow rendering" in docs
assert "<svg" in rendering and "READY" in rendering and "FALLBACK" in rendering
print("overlay recovery workflow docs contract: ok")
