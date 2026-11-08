from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'Q_PROPERTY(QString resetStatus READ resetStatus NOTIFY resetStatusChanged)',
    'resetStatus_ = QStringLiteral("Persisted recovery discarded")',
    'resetStatus_.clear()',
    'objectName: "persistedRecoveryResetStatus"',
    'storageRecovery.resetStatus',
    'void resetStatusChanged();',
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing persisted reset status markers: " + ", ".join(missing))
print(f"Persisted recovery reset status contract verified ({len(required)} checks)")
