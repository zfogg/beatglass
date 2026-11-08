"""Verify monitor recovery state is exposed through the display status control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayDisplayStatus"',
    'window.overlayDisplayRecoveryPending ? "; recovery pending"',
    'The saved display is being recovered',
    'Accessible.name: "Current overlay display: " + text',
    'Accessible.role: Accessible.StaticText',
    'objectName: "overlayDisplayEmptyState"',
    'objectName: "overlaySavedDisplayRecovery"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay screen accessibility markers: " + ", ".join(missing))
print(f"Overlay screen accessibility contract verified ({len(required)} checks)")
