"""Verify visualization mode tooltip includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationModeControls"',
    'ToolTip.text: "Visualization mode controls; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending" : "overlay is ready") + "; current mode is announced when available"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization mode tooltip markers: " + ", ".join(missing))
print(f"Visualization mode tooltip contract verified ({len(required)} checks)")
