"""Verify overlay retry tooltip includes recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: "Retry monitoring the selected output source; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing overlay retry tooltip marker")
print("Overlay retry tooltip contract verified (1 check)")
