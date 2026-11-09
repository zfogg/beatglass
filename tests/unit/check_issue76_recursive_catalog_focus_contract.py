"""Verify recursive catalog discovery focuses its result list."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert "browseRecursive(window.browseFolder, search.text)" in QML
    assert "recursiveResultsView.forceActiveFocus()" in QML
    print("Issue #76 recursive catalog focus contract verified (2 checks)")
if __name__ == "__main__": main()
