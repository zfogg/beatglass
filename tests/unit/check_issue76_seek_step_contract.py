"""Executable contract for configurable playback seek steps."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ('objectName: "seekSeconds"', "seekSeconds.value * playbackSampleRate.value", "Choose the playback seek step"):
    assert marker in qml, marker
print("Issue #76 configurable seek step contract verified")
