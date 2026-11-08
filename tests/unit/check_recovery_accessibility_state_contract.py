"""Verify persisted recovery selection exposes reset state to assistive UI."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoveryRuns"',
    'Accessible.name: "Persisted retryable analysis runs"',
    'storageRecovery.resetStatus !== undefined && storageRecovery.resetStatus !== ""',
    '"Selected persisted recovery status: " + storageRecovery.resetStatus',
    'objectName: "persistedRecoveryResetStatus"',
    'objectName: "selectedPersistedRecoveryResetStatus"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing recovery accessibility state markers: " + ", ".join(missing))
print(f"Recovery accessibility state contract verified ({len(required)} checks)")
