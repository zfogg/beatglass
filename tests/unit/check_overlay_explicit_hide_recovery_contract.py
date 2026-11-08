"""Verify an explicit user hide cancels automatic availability re-show."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function hideOverlayAndRestoreControls()",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
    "window.overlayBridge.hideOverlay()",
    "function onAvailableChanged()",
    "if (window.overlayWasVisibleBeforeAvailabilityLoss)",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing explicit hide recovery markers: " + ", ".join(missing))
if qml.index("window.overlayWasVisibleBeforeAvailabilityLoss = false") > qml.index("window.overlayBridge.hideOverlay()"):
    raise SystemExit("explicit hide must clear recovery visibility before hiding native overlay")
print(f"Overlay explicit-hide recovery contract verified ({len(required)} checks)")
