"""Verify monitor loss/restoration keeps overlay recovery explicit and safe."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property bool overlayDisplayRecoveryPending: false",
    "function recoverOverlayAfterDisplayChange()",
    "window.overlayDisplayRecoveryPending = true",
    "window.overlayBridge.moveToScreen(savedIndex)",
    "window.restoreOverlayVisualStateFromBridge()",
    "window.overlayBridge.showOverlay()",
    "DJ overlay recovered on the saved display with persisted visual settings.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay display recovery markers: " + ", ".join(missing))
if qml.count("function recoverOverlayAfterDisplayChange()") != 1:
    raise SystemExit("display recovery helper must have one canonical definition")
if qml.count("window.overlayDisplayRecoveryPending = true") < 2:
    raise SystemExit("display loss and failed recovery must both remain pending")
print(f"Overlay display recovery contract verified ({len(required)} checks)")
