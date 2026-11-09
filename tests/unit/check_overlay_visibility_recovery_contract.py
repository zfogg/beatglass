"""Verify availability recovery only restores overlay visibility when needed."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property bool overlayWasVisibleBeforeAvailabilityLoss: false",
    "window.overlayWasVisibleBeforeAvailabilityLoss = djOverlay.visible",
    "window.hideOverlayAndRestoreControls()",
    "if (window.overlayWasVisibleBeforeAvailabilityLoss)",
    "window.recoverOverlayAfterDisplayChange()",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay visibility recovery markers: " + ", ".join(missing))
print(f"Overlay visibility recovery contract verified ({len(required)} checks)")
