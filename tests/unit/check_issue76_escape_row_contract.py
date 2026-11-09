"""Verify Escape clears the Library browser row with its source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.clearSelectedAudio(); browser.currentIndex = -1' in QML
    print("Issue #76 Escape row contract verified")
if __name__ == "__main__": main()
