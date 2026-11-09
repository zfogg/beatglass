"""Verify playback feedback description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Playback and seek readiness for the accepted waveform or FFT snapshot; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing playback feedback description marker")
print("Visualization playback description contract verified (1 check)")
