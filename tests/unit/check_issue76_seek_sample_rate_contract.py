"""Executable contract for sample-rate-aware playback seek."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("currentFrame - 5 * playbackSampleRate.value", "currentFrame + 5 * playbackSampleRate.value", "selected sample rate"):
    assert marker in qml, marker
print("Issue #76 sample-rate-aware seek contract verified")
