"""Contract checks for Library playback action focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryPlaybackAction")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "playSelectedTrack.forceActiveFocus()" in block
    assert "Selected Library playback action focused" in block
    assert "window.focusLibraryPlaybackAction()" in QML
if __name__ == "__main__":
    main()
