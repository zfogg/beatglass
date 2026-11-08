"""Verify selecting a display reapplies persisted geometry and opacity."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function persistOverlayDisplaySelection(index, name)",
    "window.overlayBridge.moveToScreen(index)",
    "if (!window.restoreOverlayVisualStateFromBridge())",
    "Overlay display selected, but persisted visual state could not be reapplied.",
    "window.overlayDisplayRecoveryPending = false",
    "DJ overlay display selection persisted for ",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display visual reapply markers: " + ", ".join(missing))
start = qml.index("function persistOverlayDisplaySelection(index, name)")
end = qml.index("function hideOverlayAndRestoreControls()")
segment = qml[start:end]
if segment.index("window.restoreOverlayVisualStateFromBridge()") < segment.index("window.overlayBridge.moveToScreen(index)"):
    raise SystemExit("display move must precede visual-state reapply")
print(f"Overlay display visual reapply contract verified ({len(required)} checks)")
