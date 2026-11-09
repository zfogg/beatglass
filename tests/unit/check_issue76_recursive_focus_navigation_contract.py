"""Verify recursive focus shortcut returns navigation to Library."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'navigation.currentIndex = 0; recursiveResultsView.forceActiveFocus()' in QML
    print("Issue #76 recursive focus navigation contract verified")
if __name__ == "__main__": main()
