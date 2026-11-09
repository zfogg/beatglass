"""Verify display enumeration changes route focus to the next recovery action."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onScreensChanged()",
    "overlayDisplay.forceActiveFocus()",
    "retryOverlay.forceActiveFocus()",
    "overlay actions disabled and controls restored until a display is available.",
    "choose a display to re-enable overlay actions and resume monitoring.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display enumeration focus markers: " + ", ".join(missing))
if qml.count("overlayDisplay.forceActiveFocus()") < 2:
    raise SystemExit("both display loss and unknown replacement targets must focus the selector")
print(f"Overlay enumeration focus contract verified ({len(required)} checks)")
