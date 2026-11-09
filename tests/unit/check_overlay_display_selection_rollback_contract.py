"""Verify rejected display selection restores the native selection."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("const requestedIndex = currentIndex", "window.persistOverlayDisplaySelection(requestedIndex, currentText)", "currentIndex = window.overlayBridge && window.overlayBridge.screenIndex >= 0 ? window.overlayBridge.screenIndex : 0"):
    if marker not in qml:
        raise SystemExit(f"Missing display rollback marker: {marker}")
print("Overlay display selection rollback contract verified (3 checks)")
