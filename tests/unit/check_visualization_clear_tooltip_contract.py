"""Verify clear-selection tooltip includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: (enabled ? "Clear the selected visualization position" : "No visualization position is selected") + "; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing clear-selection tooltip marker")
print("Visualization clear tooltip contract verified (1 check)")
