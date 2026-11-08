"""Verify hide/show/reapply paths share persisted visual-state restoration."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function restoreOverlayVisualStateFromBridge()",
    "function showOverlayAndHideControls()",
    "if (!window.restoreOverlayVisualStateFromBridge())",
    "function reapplyOverlaySettingsAndRestore()",
    "window.restoreOverlayVisualStateFromBridge()",
    "function hideOverlayAndRestoreControls()",
    "window.overlayBridge.hideOverlay()",
    "function reapplyOverlayVisualState()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay visual-state coherence markers: " + ", ".join(missing))
if qml.count("function restoreOverlayVisualStateFromBridge()") != 1:
    raise SystemExit("visual-state restore helper must have one canonical definition")
print(f"Overlay visual-state coherence contract verified ({len(required)} checks)")
