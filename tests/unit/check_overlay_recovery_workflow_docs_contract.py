from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = Path("docs/overlay-recovery-workflow.md").read_text(encoding="utf-8")

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
print("overlay recovery workflow docs contract: ok")
