"""Contract checks for Library recovery-next-action focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryRecoveryNextAction")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "analysisRecoveryNextAction.forceActiveFocus()" in block
    assert "Library recovery next action focused" in block
    assert "window.focusLibraryRecoveryNextAction()" in QML
if __name__ == "__main__":
    main()
