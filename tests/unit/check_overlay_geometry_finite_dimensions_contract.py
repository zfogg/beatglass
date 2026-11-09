"""Verify persisted overlay dimensions reject non-finite values."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
if qml.count("!isFinite(geometry.width)") < 1 or qml.count("!isFinite(window.overlayBridge.geometry.width)") < 1:
    raise SystemExit("Missing finite overlay dimension validation")
print("Overlay geometry finite dimensions contract verified (2 checks)")
