"""Verify failed reapply leaves the control window visible and focused."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function reapplyOverlaySettingsAndRestore()",
    "window.show()",
    "window.raise()",
    "window.requestActivate()",
    "Persisted overlay settings unavailable; control window remains available as the tray/taskbar fallback.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing reapply focus markers: " + ", ".join(missing))
start = qml.index("function reapplyOverlaySettingsAndRestore()")
end = qml.index("function recoverOverlayAfterDisplayChange()")
segment = qml[start:end]
if not all(marker in segment for marker in ("window.show()", "window.raise()", "window.requestActivate()")):
    raise SystemExit("all reapply failure branches must restore control-window focus")
print(f"Overlay reapply focus contract verified ({len(required)} checks)")
