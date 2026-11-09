"""Ensure persisted retry rejection diagnostics reach the recovery UI."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoveryRejectionReason"',
    'storageRecovery.status.indexOf("Retry rejected:")',
    'RetryRejectionReason::stale_source_identity',
    'retry_rejection_message',
]
missing = [marker for marker in required if marker not in qml + bridge]
if missing:
    raise SystemExit("Missing persisted retry rejection UI markers: " + ", ".join(missing))
print(f"Persisted retry rejection UI contract verified ({len(required)} checks)")
