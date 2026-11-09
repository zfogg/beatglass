"""Verify visualization status description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Current waveform or FFT availability, generation, and capture state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing visualization status description marker")
print("Visualization status description contract verified (1 check)")
