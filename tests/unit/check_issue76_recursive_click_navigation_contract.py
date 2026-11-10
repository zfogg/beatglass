"""Verify recursive result activation selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'recursiveResultsView.currentIndex = index\n                                navigation.currentIndex = 0' in QML
    print("Issue #76 recursive click navigation contract verified")
if __name__ == "__main__": main()
