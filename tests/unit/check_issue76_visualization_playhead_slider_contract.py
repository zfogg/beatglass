"""Executable contract for the time-aligned visualization playhead slider."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "visualizationPositionSlider"')
block = qml[start:start + 1200]
for marker in ("visualizationCanvas.selectedPosition", "onMoved", "Choose the waveform playhead position"):
    assert marker in block, marker
print("Issue #76 visualization playhead slider contract verified")
