"""Verify double-clicking recursive audio starts playback."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("required property bool folder", "onDoubleClicked: if (!folder)", "onClicked(); playSelectedTrack.click()"):
        assert marker in QML, marker
    print("Issue #76 recursive double-click play contract verified (3 checks)")
if __name__ == "__main__": main()
