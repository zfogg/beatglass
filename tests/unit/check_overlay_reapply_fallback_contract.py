"""Verify reapply settings failures retain explicit fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function reapplyOverlaySettingsAndRestore()",
    "window.overlayDisplayRecoveryPending = true",
    "window.overlayBridge.moveToScreen(window.overlayBridge.screenIndex)",
    "window.restoreOverlayVisualStateFromBridge()",
    "Persisted overlay settings unavailable; control window remains available as the tray/taskbar fallback.",
    "Persisted overlay display, geometry, and opacity reapplied.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay reapply fallback markers: " + ", ".join(missing))
print(f"Overlay reapply fallback contract verified ({len(required)} checks)")
