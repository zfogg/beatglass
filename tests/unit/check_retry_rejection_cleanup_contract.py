"""Verify bridge/UI retry rejection state is replaced after resolution."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'if (ok) refresh();',
    'QStringLiteral("Retry queued")',
    'objectName: "persistedRecoveryRejectionReason"',
    'storageRecovery.status.indexOf("Retry rejected:") >= 0',
    'Retryable run discarded',
]
missing = [marker for marker in required if marker not in bridge + qml]
if missing:
    raise SystemExit("Missing retry rejection cleanup markers: " + ", ".join(missing))
print(f"Retry rejection cleanup contract verified ({len(required)} checks)")
