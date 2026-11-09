"""Verify explicit show restores control focus when persisted visuals are invalid."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function showOverlayAndHideControls()",
    "if (!window.restoreOverlayVisualStateFromBridge())",
    "window.overlayDisplayRecoveryPending = true",
    "window.show()",
    "window.raise()",
    "window.requestActivate()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing explicit-show visual fallback markers: " + ", ".join(missing))
print(f"Overlay show visual fallback contract verified ({len(required)} checks)")
