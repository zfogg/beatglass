"""Verify device recovery tooltip includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: "Capture device and permission recovery state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing device recovery tooltip marker")
print("Visualization device tooltip contract verified (1 check)")
