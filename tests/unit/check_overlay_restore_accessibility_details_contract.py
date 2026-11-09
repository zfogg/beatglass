"""Verify persisted restore accessibility text describes all settings."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayRestoreState"',
    'Accessible.description: "Restored overlay display, size, and opacity',
    'including whether recovery is pending',
    'ToolTip.text: "Persisted display, size, opacity, and recovery state',
    'Accessible.role: Accessible.Status',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing restore accessibility detail markers: " + ", ".join(missing))
print(f"Overlay restore accessibility details contract verified ({len(required)} checks)")
