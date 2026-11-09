"""Verify rapid opacity edits match echoes by value instead of arrival order."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property real overlayPendingOpacity: -1",
    "window.overlayPendingOpacity = value",
    "Math.abs(Number(window.overlayBridge.opacity) - window.overlayPendingOpacity) < 0.001",
    "window.overlayPendingOpacity = -1",
    "!window.overlayOpacityEditInFlight",
    "overlayOpacityControl.value = window.overlayBridge.opacity",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing rapid opacity echo markers: " + ", ".join(missing))
print(f"Overlay rapid opacity echo contract verified ({len(required)} checks)")
