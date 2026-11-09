"""Verify Library refresh clears the selected audio source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert QML.count('window.clearSelectedAudio(); browser.currentIndex = -1') >= 2
    print("Issue #76 refresh audio reset contract verified")
if __name__ == "__main__": main()
