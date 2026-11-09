"""Verify clear-selection description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationClearSelection"',
    '"Clear the selected waveform or FFT position; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")',
    '"There is no selected waveform or FFT position to clear; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing clear description markers: " + ", ".join(missing))
print(f"Visualization clear description contract verified ({len(required)} checks)")
