"""Executable contract for presenting decoded track metadata in details."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "selectedAudioDuration"')
block = qml[start:start + 700]
for marker in ("metadataStatus.indexOf", "Metadata: ", "Duration: unknown"):
    assert marker in block, marker
print("Issue #76 metadata duration presentation contract verified")
