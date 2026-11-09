"""Verify geometry/DPI changes can complete pending monitor recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onGeometryChanged()",
    "window.overlayDisplayRecoveryPending",
    "window.overlayBridge.screenNames.length > 0",
    "!window.overlayDisplaySelectionRequired",
    "window.recoverOverlayAfterDisplayChange()",
    "retryOverlay.forceActiveFocus()",
    "window.syncOverlayGeometryStateFromBridge()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry monitor recovery markers: " + ", ".join(missing))
print(f"Overlay geometry monitor recovery contract verified ({len(required)} checks)")
