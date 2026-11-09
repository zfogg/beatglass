"""Verify failed overlay show activates fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (!window.overlayBridge.showOverlay()) {\n            window.overlayLaunchFallbackActive = true'
if marker not in qml:
    raise SystemExit("Missing show-failure fallback state")
print("Overlay show fallback state contract verified (1 check)")
