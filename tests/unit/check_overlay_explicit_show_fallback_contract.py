"""Verify explicit show clears stale recovery intent and restores fallback on failure."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function showOverlayAndHideControls()",
    "window.overlayDisplayRecoveryPending = true",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
    "window.overlayDisplayRecoveryPending = false",
    "if (!window.overlayBridge.showOverlay())",
    "window.show()",
    "window.requestActivate()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing explicit show fallback markers: " + ", ".join(missing))
print(f"Overlay explicit-show fallback contract verified ({len(required)} checks)")
