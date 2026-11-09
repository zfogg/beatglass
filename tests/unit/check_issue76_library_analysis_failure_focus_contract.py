"""Contract checks for Library analysis failure focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryAnalysisFailure")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "analysisFailureReason.forceActiveFocus()" in block
    assert "Library analysis failure status focused" in block
    assert "window.focusLibraryAnalysisFailure()" in QML
if __name__ == "__main__":
    main()
