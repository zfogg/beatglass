"""Verify double-clicking folders opens them and audio files plays them."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onDoubleClicked: { browser.currentIndex = index; window.activateEntry(index); if (!fileIsDir) playSelectedTrack.click() }' in QML
    print("Issue #76 folder double-click contract verified")
if __name__ == "__main__": main()
