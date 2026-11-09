"""Verify waveform and FFT descriptions include overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "waveformView"',
    '"Waveform view is selected; show the selectable time-domain waveform." : "Show the selectable time-domain waveform.") + (window.overlayDisplayRecoveryPending',
    'objectName: "spectrumView"',
    '"FFT view is selected; show the selectable frequency-domain FFT." : "Show the selectable frequency-domain FFT.") + (window.overlayDisplayRecoveryPending',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing mode button description markers: " + ", ".join(missing))
print(f"Visualization mode button descriptions contract verified ({len(required)} checks)")
