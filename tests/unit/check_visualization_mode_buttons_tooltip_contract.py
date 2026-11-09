"""Verify waveform and FFT tooltips include overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'ToolTip.text: "Show the time-domain waveform (Ctrl+1); " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'ToolTip.text: "Show the frequency-domain FFT (Ctrl+2); " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing mode button tooltip markers: " + ", ".join(missing))
print(f"Visualization mode button tooltips contract verified ({len(required)} checks)")
