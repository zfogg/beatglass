"""Verify batch retry/discard controls preserve keyboard focus and recovery announcements."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "retryAnalysisBatch"',
    "retryAnalysisBatchButton.forceActiveFocus()",
    'objectName: "discardAnalysisBatch"',
    "discardAnalysisBatchButton.forceActiveFocus()",
    "onRejected: discardAnalysisBatchButton.forceActiveFocus()",
    "Confirmation is required; focus returns here after the dialog.",
    "Analysis batch retry queued; focus will return",
    "Analysis batch recovery discarded; focus remains",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing batch recovery focus markers: " + ", ".join(missing))
print(f"Batch recovery focus contract verified ({len(required)} checks)")
