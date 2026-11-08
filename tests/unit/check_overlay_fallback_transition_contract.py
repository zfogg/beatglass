"""Verify failed overlay transitions restore a discoverable control-window fallback."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function restoreOverlayFallbackControls(reason)",
    "window.show()",
    "window.raise()",
    "window.requestActivate()",
    "window.restoreOverlayFallbackControls(\"DJ overlay unavailable; control window remains available as the tray/taskbar fallback.\")",
    "window.restoreOverlayFallbackControls(\"DJ overlay visual state unavailable; control window remains available as the tray/taskbar fallback.\")",
    "DJ overlay could not be restored; control window remains available as the tray/taskbar fallback.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay fallback transition markers: " + ", ".join(missing))
print(f"Overlay fallback transition contract verified ({len(required)} checks)")
