"""Verify overlay geometry synchronization rejects incomplete rectangles."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = "window.overlayBridge.geometry.width <= 0\n                || window.overlayBridge.geometry.height <= 0"
if marker not in qml:
    raise SystemExit("Missing overlay geometry width/height validation")
print("Overlay geometry validation contract verified (1 check)")
