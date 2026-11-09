"""Verify display-selection failures preserve actionable fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function persistOverlayDisplaySelection(index, name)",
    "window.overlayDisplayRecoveryPending = true",
    "window.overlayBridge.moveToScreen(index)",
    "Overlay display selection could not be applied; control-window fallback remains active.",
    "window.show()",
    "window.raise()",
    "window.requestActivate()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display selection fallback markers: " + ", ".join(missing))
start = qml.index("function persistOverlayDisplaySelection(index, name)")
end = qml.index("function hideOverlayAndRestoreControls()")
segment = qml[start:end]
if not all(marker in segment for marker in ("window.show()", "window.raise()", "window.requestActivate()")):
    raise SystemExit("display selection failure must restore control-window focus")
if segment.count("window.overlayLaunchFallbackActive = true") != 4:
    raise SystemExit("display selection failures must activate control-window fallback")
print(f"Overlay display selection fallback contract verified ({len(required)} checks)")
