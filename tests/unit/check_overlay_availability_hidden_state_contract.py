"""Verify availability loss does not masquerade as user-hidden state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.hideOverlayAndRestoreControls()\n                    window.overlayHiddenByUser = false'
if marker not in qml:
    raise SystemExit("Missing availability hidden-state correction")
print("Overlay availability hidden-state contract verified (1 check)")
