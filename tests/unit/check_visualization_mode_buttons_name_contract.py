"""Verify waveform and FFT controls name overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "waveformView"',
    'Accessible.name: "Waveform view, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'objectName: "spectrumView"',
    'Accessible.name: "FFT view, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization mode button markers: " + ", ".join(missing))
print(f"Visualization mode buttons contract verified ({len(required)} checks)")
