"""Contract checks that reopen shortcut uses the shared playback lifecycle."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function reopenLibrarySelectionFromShortcut")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "reopenSelectedLibraryTrack()" in block
    assert "reopenSelectedPlayback.forceActiveFocus()" in block
    assert "window.reopenLibrarySelectionFromShortcut()" in QML
if __name__ == "__main__":
    main()
