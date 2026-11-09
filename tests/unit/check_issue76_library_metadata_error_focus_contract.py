"""Contract checks for metadata-error guidance focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryMetadataErrorGuidance")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "metadataErrorGuidance.forceActiveFocus()" in block
    assert "Library metadata error guidance focused" in block
    assert "window.focusLibraryMetadataErrorGuidance()" in QML
if __name__ == "__main__":
    main()
