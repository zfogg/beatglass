"""Verify Ctrl+Alt+R starts the recursive Library catalog."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "libraryRecursiveCatalogShortcut"' in QML
    assert 'sequence: "Ctrl+Alt+R"' in QML
    assert 'id: addFolderRecursively' in QML
    assert 'onActivated: addFolderRecursively.click()' in QML
    print("Issue #76 recursive catalog shortcut contract verified")
if __name__ == "__main__": main()
