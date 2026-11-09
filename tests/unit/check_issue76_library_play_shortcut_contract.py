"""Contract checks that play shortcut uses shared playback behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function playLibrarySelectionFromShortcut")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "playSelectedLibraryTrack()" in block
    assert "playSelectedTrack.forceActiveFocus()" in block
    assert "window.playLibrarySelectionFromShortcut()" in QML
if __name__ == "__main__":
    main()
