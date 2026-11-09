"""Verify recovery announcements name overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationRecoveryAnnouncement"',
    'Accessible.name: "Visualization recovery announcement, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text',
    'Accessible.value: text',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization announcement markers: " + ", ".join(missing))
print(f"Visualization announcement name contract verified ({len(required)} checks)")
