"""Verify waveform and FFT values include overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "waveformView"; checkable: true; checked: visualizationCanvas.viewMode === "waveform"; Accessible.value: (checked ? "selected" : "not selected") + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")',
    'objectName: "spectrumView"; checkable: true; checked: visualizationCanvas.viewMode === "spectrum"; Accessible.value: (checked ? "selected" : "not selected") + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization mode button value markers: " + ", ".join(missing))
print(f"Visualization mode button values contract verified ({len(required)} checks)")
