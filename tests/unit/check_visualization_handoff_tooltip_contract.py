"""Verify analysis handoff tooltip includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: "Accepted analysis generation and revision consumed by the visualization; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing analysis handoff tooltip marker")
print("Visualization handoff tooltip contract verified (1 check)")
