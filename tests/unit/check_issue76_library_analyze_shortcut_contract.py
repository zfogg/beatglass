"""Contract checks for shared Library analysis shortcut behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function analyzeLibrarySelectionFromShortcut")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "submitSelectedFile(window.selectedAudioUrl.toLocalFile())" in block
    assert "clearVisualizationForCaptureState" in block
    assert "analyzeSelectedAudio.forceActiveFocus()" in block
    assert "window.analyzeLibrarySelectionFromShortcut()" in QML
if __name__ == "__main__":
    main()
