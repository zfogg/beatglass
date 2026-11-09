"""Verify recursive cataloging clears the folder browser row."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.clearSelectedAudio()\n                            browser.currentIndex = -1' in QML
    print("Issue #76 recursive catalog row reset contract verified")
if __name__ == "__main__": main()
