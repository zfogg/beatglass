"""Verify visualization mode controls name overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationModeControls"',
    'Accessible.name: "Visualization mode controls, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: visualizationCanvas.viewMode',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization mode controls markers: " + ", ".join(missing))
print(f"Visualization mode controls contract verified ({len(required)} checks)")
