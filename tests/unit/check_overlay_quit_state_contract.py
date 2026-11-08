"""Verify quitting cancels recovery intent before hiding the native overlay."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function quitApplication()",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
    "window.overlayDisplayRecoveryPending = false",
    "window.overlayBridge.hideOverlay()",
    "Beatglass is quitting; overlay hidden and persisted settings retained.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay quit-state markers: " + ", ".join(missing))
if qml.index("window.overlayDisplayRecoveryPending = false", qml.index("function quitApplication()")) > qml.index("window.overlayBridge.hideOverlay()", qml.index("function quitApplication()")):
    raise SystemExit("quit must clear recovery state before hiding native overlay")
print(f"Overlay quit-state contract verified ({len(required)} checks)")
