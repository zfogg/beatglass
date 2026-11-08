"""Verify failed/closed storage exposes an actionable reopen control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "reopenStorage"',
    'storageRecovery.status === "Storage closed"',
    'storageRecovery.status.indexOf("Storage error:") === 0',
    'storageRecovery.open()',
    '"Storage reopen requested; persisted recovery will refresh when the database is available."',
    'Accessible.name: "Reopen analysis storage"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing storage reopen QML markers: " + ", ".join(missing))
print(f"Storage reopen QML contract verified ({len(required)} checks)")
