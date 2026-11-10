"""Verify changing the Library filter clears the browser row."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onTextChanged: { window.refreshCatalog(); browser.currentIndex = -1 }' in QML
    print("Issue #76 filter selection reset contract verified")
if __name__ == "__main__": main()
