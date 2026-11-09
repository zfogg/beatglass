"""Contract checks for shared Library sort action behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function toggleLibrarySort")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "sortDescending" in block
    assert "announceVisualizationRecovery" in block
    assert "target.forceActiveFocus()" in block
    assert "window.toggleLibrarySort(sortOrder)" in QML
if __name__ == "__main__":
    main()
