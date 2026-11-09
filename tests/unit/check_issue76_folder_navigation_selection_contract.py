"""Verify opening a Library folder clears the prior row selection."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'browseFolder = url\n            browser.currentIndex = -1' in QML
    print("Issue #76 folder navigation selection contract verified")
if __name__ == "__main__": main()
