"""Verify fallback controls clear stale user-hidden overlay state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'function restoreOverlayFallbackControls(reason) {\n        window.overlayHiddenByUser = false'
if marker not in qml:
    raise SystemExit("Missing fallback hidden-state reset")
print("Overlay fallback hidden-state contract verified (1 check)")
