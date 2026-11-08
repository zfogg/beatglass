"""Verify overlay action group communicates availability and recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayActions"',
    'window.overlayDisplayRecoveryPending ?', 
    '"unavailable; control-window fallback active"',
    '"recovery is pending and the control-window fallback remains available."',
    '"Overlay actions are available; recovery is pending"',
    '"Overlay window actions are unavailable; control-window fallback is active"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay action state markers: " + ", ".join(missing))
print(f"Overlay actions state contract verified ({len(required)} checks)")
