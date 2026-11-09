"""Verify batch recovery is visible from the full Library surface."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "libraryBatchRecoveryBanner"',
    'window.analysisJobBridge.batchAcceptedCount',
    'window.analysisJobBridge.batchCompletedCount',
    'window.analysisJobBridge.batchFailedCount',
    'window.analysisJobBridge.batchCancelledCount',
    'Retry or discard the retained batch in the selected source panel',
    'objectName: "analysisRecoveryNextAction"',
    'retryAnalysisBatchButton.forceActiveFocus()',
    'discardAnalysisBatchButton.forceActiveFocus()',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise AssertionError("missing Library recovery banner markers: " + ", ".join(missing))
print(f"Library batch recovery banner contract: {len(required)} checks passed")
