"""Verify recovery UI selection/diagnostics reset after persisted run cleanup."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "retryableRunIds_ = ids; retryableRunDetails_ = details;",
    "!retryableRunIds_.contains(selectedRunId_)",
    "selectedRunId_.clear(); emit selectedRunChanged();",
    'objectName: "persistedRecoveryRejectionReason"',
    'storageRecovery.status.indexOf("Retry rejected:") >= 0',
]
missing = [marker for marker in required if marker not in bridge + qml]
if missing:
    raise SystemExit("Missing recovery UI reset markers: " + ", ".join(missing))
print(f"Recovery UI reset contract verified ({len(required)} checks)")
