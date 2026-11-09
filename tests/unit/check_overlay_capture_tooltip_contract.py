"""Verify overlay capture tooltip exposes recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: "Current output capture status; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing overlay capture tooltip recovery marker")
print("Overlay capture tooltip contract verified (1 check)")
