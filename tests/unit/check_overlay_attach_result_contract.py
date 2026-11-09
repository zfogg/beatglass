"""Verify startup handles native overlay attachment failures."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (overlayBridge && !overlayBridge.attach(djOverlay, window))'
if marker not in qml:
    raise SystemExit("Missing overlay attach result handling")
print("Overlay attach result contract verified (1 check)")
