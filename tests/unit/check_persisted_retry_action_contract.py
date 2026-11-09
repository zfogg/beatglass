"""Verify persisted retry selection is gated by explicit retryable state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'objectName: "retryPersistedRecovery"',
    'storageRecovery.selectedRunId !== "" && storageRecovery.analysisRetryable',
    "Q_PROPERTY(bool analysisRetryable",
    "retrySelectedRun()",
    "retryableRunIds",
]
missing = [marker for marker in required if marker not in qml + bridge]
if missing:
    raise SystemExit("Missing persisted retry action markers: " + ", ".join(missing))
if "Retry rejected: no persisted run selected" not in source:
    raise SystemExit("Retry without selection must report a rejection")
print(f"Persisted retry action contract verified ({len(required)} checks)")
