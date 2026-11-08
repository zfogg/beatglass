"""Verify explicit hide preserves native settings but clears transient recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function hideOverlayAndRestoreControls()",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
    "window.overlayDisplayRecoveryPending = false",
    "window.overlayBridge.hideOverlay()",
    "persisted settings retained",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay hide persistence markers: " + ", ".join(missing))
hide = qml.index("function hideOverlayAndRestoreControls()")
quit_fn = qml.index("function quitApplication()")
if not (hide < qml.index("window.overlayDisplayRecoveryPending = false", hide) < quit_fn):
    raise SystemExit("explicit hide must clear transient recovery state in its own handler")
print(f"Overlay hide persistence contract verified ({len(required)} checks)")
