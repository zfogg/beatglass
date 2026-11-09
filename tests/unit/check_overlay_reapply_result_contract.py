"""Verify persisted overlay geometry and opacity failures abort reapply."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("if (!window.overlayBridge.resizeOverlay(savedGeometry.width, savedGeometry.height))", "if (!window.overlayBridge.setOpacity(savedOpacity))", "return false"):
    if marker not in qml:
        raise SystemExit(f"Missing reapply result marker: {marker}")
print("Overlay reapply result contract verified (3 checks)")
