"""Verify hide action derives hidden state and announcement from native result."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("const hidden = !window.overlayBridge || window.overlayBridge.hideOverlay()", "window.overlayHiddenByUser = hidden", "window.announceCaptureRecoveryState(hidden"):
    if marker not in qml:
        raise SystemExit(f"Missing hide state marker: {marker}")
print("Overlay hide state contract verified (3 checks)")
