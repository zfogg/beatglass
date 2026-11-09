"""Verify the recursive results focus shortcut."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "libraryRecursiveResultsFocusShortcut"' in QML
    assert 'sequence: "Ctrl+Alt+Shift+R"' in QML
    assert 'recursiveResultsView.forceActiveFocus()' in QML
    print("Issue #76 recursive results focus contract verified")
if __name__ == "__main__": main()
