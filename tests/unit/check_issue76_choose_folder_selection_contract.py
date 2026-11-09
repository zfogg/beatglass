"""Verify choosing a Library folder resets filter and row selection."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.browseFolder = selectedFolder\n            search.text = ""\n            browser.currentIndex = -1' in QML
    print("Issue #76 choose folder selection contract verified")
if __name__ == "__main__": main()
