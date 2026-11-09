"""Verify overlay retry clears stale user-hidden state before recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'function showOverlayAndHideControls() {\n        window.overlayHiddenByUser = false'
if marker not in qml:
    raise SystemExit("Missing overlay retry hidden-state reset")
print("Overlay retry hidden-state contract verified (1 check)")
