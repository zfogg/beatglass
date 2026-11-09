"""Verify storage open/close failures are visible and actionable in QML."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "storageTransitionStatus"',
    'storageRecovery.status === "Opening storage"',
    'storageRecovery.status === "Storage closed"',
    'storageRecovery.status.indexOf("Storage error:") === 0',
    '"Storage transition: " + storageRecovery.status',
    'Accessible.role: Accessible.Status',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing storage transition QML markers: " + ", ".join(missing))
print(f"Storage transition QML contract verified ({len(required)} checks)")
