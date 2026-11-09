"""Executable contract for selecting recursive Library results."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in (
    'objectName: "recursiveResultsFrame"',
    'objectName: "browserFrame"\n                    Layout.fillWidth: true; Layout.fillHeight: true\n                    visible: !window.recursiveCatalogActive',
    'window.metadataBridge.inspect(window.selectedAudioUrl)',
    'Select this audio file for metadata and analysis',
):
    assert marker in qml, marker
print("Issue #76 recursive result selection contract verified")
