"""Verify visualization mode description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationModeControls"',
    'Accessible.description: "Choose waveform or frequency-domain FFT visualization; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization mode description markers: " + ", ".join(missing))
print(f"Visualization mode description contract verified ({len(required)} checks)")
