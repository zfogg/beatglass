"""Verify persisted overlay geometry rejects non-finite positions."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
if qml.count("!isFinite(geometry.x)") < 1 or qml.count("!isFinite(window.overlayBridge.geometry.x)") < 1:
    raise SystemExit("Missing finite overlay position validation")
print("Overlay geometry finite contract verified (2 checks)")
