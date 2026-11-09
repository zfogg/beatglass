"""Verify persisted recovery selection state is visible and actionable."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoverySelectionStatus"',
    'storageRecovery.selectedRunId !== ""',
    'storageRecovery.analysisRetryable ? " · retry available"',
    'objectName: "persistedRecoveryRuns"',
    'storageRecovery.selectRun(currentText)',
    'objectName: "discardPersistedRecovery"',
    'objectName: "retryPersistedRecovery"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise AssertionError("missing persisted recovery status markers: " + ", ".join(missing))
print(f"Persisted recovery selection status contract: {len(required)} checks passed")
