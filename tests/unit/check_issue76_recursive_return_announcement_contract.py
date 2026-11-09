"""Verify the recursive return shortcut announces its focus transition."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'libraryRecursiveReturnShortcut' in QML
    assert 'window.announceVisualizationRecovery("Returned to the current folder browser.")' in QML
    print("Issue #76 recursive return announcement contract verified")
if __name__ == "__main__": main()
