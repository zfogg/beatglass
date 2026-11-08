"""Verify native overlay state is finalized when the control window closes."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "onClosing: {",
    "window.overlayWasVisibleBeforeAvailabilityLoss = false",
    "window.overlayDisplayRecoveryPending = false",
    "if (overlayBridge) overlayBridge.hideOverlay()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay window-close markers: " + ", ".join(missing))
close = qml.index("onClosing: {")
if not (close < qml.index("window.overlayDisplayRecoveryPending = false", close) < qml.index("overlayBridge.hideOverlay()", close)):
    raise SystemExit("window close must clear transient state before native hide")
print(f"Overlay window-close contract verified ({len(required)} checks)")
