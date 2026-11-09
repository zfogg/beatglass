"""Contract checks for the shared Library browser selection reset workflow."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function resetLibraryBrowserSelection")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "window.clearSelectedAudio()" in block
    assert "browser.currentIndex = -1" in block
    assert "announceVisualizationRecovery(message)" in block
    assert "browser.forceActiveFocus()" in block
    refresh = QML[QML.index('objectName: "refreshFolder"'):QML.index('objectName: "folderPath"')]
    assert "window.resetLibraryBrowserSelection" in refresh
if __name__ == "__main__":
    main()
