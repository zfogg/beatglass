"""Verify the recursive results return shortcut."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "libraryRecursiveReturnShortcut"' in QML
    assert 'sequence: "Ctrl+Alt+Shift+B"' in QML
    assert 'onActivated: { window.recursiveCatalogActive = false' in QML
    print("Issue #76 recursive return shortcut contract verified")
if __name__ == "__main__": main()
