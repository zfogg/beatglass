"""Verify selection reset status names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationSelectionResetFeedback"',
    'Accessible.name: "Visualization selection reset status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: text',
    'ToolTip.text: "Why the visualization selection was cleared"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing selection reset markers: " + ", ".join(missing))
print(f"Selection reset name contract verified ({len(required)} checks)")
