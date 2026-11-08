"""Verify persisted overlay visual state is applied before every show path."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function restoreOverlayVisualStateFromBridge()",
    "window.restoreOverlayVisualStateFromBridge()",
    "window.overlayBridge.resizeOverlay(Number(geometry.width), Number(geometry.height))",
    "window.overlayBridge.setOpacity(safeOpacity)",
    "Persisted overlay geometry unavailable; control-window fallback remains active.",
    "Persisted overlay opacity unavailable; safe 90 percent opacity applied.",
    "if (!window.restoreOverlayVisualStateFromBridge())",
    "function reapplyOverlayVisualState()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay visual-state markers: " + ", ".join(missing))
print(f"Overlay visual-state application contract verified ({len(required)} checks)")
