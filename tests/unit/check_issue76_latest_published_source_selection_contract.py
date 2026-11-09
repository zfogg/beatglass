"""Executable contract for selecting the latest published batch source."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "selectLatestPublishedSource"')
block = qml[start:start + 1800]
for marker in ("batchLastSource", "selectedAudioUrl", "selectedName", "metadataBridge.inspect", "Latest published analysis source selected"):
    assert marker in block, marker
print("Issue #76 latest published source selection contract verified")
