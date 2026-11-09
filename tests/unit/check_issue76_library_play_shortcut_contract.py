"""Verify Ctrl+Alt+P plays the selected Library source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"libraryPlayShortcut\"", "sequence: \"Ctrl+Alt+P\"", "window.playbackBridge.openSource(window.selectedAudioUrl", "playSelectedTrack.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 Library play shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
