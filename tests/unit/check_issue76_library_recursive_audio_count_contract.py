"""Contract checks that recursive analysis is gated by audio results."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert "recursiveAudioResultCount" in QML
    start = QML.index('objectName: "analyzeRecursiveResults"')
    block = QML[start:QML.index('objectName: "returnToFolderBrowser"', start)]
    assert "window.recursiveAudioResultCount > 0" in block
    assert 'Accessible.value: enabled ? "available; " + window.recursiveAudioResultCount' in block
if __name__ == "__main__":
    main()
