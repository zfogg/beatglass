"""Verify folder selection announces and returns focus to the browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("window.browseFolder = selectedFolder", "Library folder selected; choose an audio file", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 folder selection focus contract verified (3 checks)")
if __name__ == "__main__": main()
