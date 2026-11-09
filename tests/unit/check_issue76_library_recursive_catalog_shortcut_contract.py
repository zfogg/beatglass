"""Contract checks for recursive catalog shortcut behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function requestLibraryRecursiveCatalog")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "addFolderRecursively.click()" in block
    assert "Recursive Library catalog requested" in block
    assert "window.requestLibraryRecursiveCatalog()" in QML
if __name__ == "__main__":
    main()
