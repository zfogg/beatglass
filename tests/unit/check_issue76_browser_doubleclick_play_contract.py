"""Verify double-clicking a browser audio item starts playback."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("onDoubleClicked: if (!fileIsDir)", "window.activateEntry(index); playSelectedTrack.click()"):
        assert marker in QML, marker
    print("Issue #76 browser double-click play contract verified (2 checks)")
if __name__ == "__main__": main()
