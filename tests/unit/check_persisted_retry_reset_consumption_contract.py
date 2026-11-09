"""Verify selected persisted retry controls consume reset-status semantics."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoveryResetStatus"',
    'storageRecovery.resetStatus',
    'objectName: "retryPersistedRecovery"',
    "A persisted discard status clears when retry is admitted.",
    'Q_PROPERTY(QString resetStatus READ resetStatus NOTIFY resetStatusChanged)',
    'if (ok && text == QStringLiteral("Retry queued") && !resetStatus_.isEmpty())',
]
missing = [marker for marker in required if marker not in qml + bridge + source]
if missing:
    raise SystemExit("Missing persisted retry reset consumption markers: " + ", ".join(missing))
print(f"Persisted retry reset consumption contract verified ({len(required)} checks)")
