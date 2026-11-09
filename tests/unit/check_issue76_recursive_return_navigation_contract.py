"""Verify recursive return shortcut selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'recursiveResultsView.currentIndex = -1; navigation.currentIndex = 0' in QML
    print("Issue #76 recursive return navigation contract verified")
if __name__ == "__main__": main()
