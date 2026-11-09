"""Verify Reopen selected is guarded and result-aware."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("function reopenSelectedLibraryTrack()", "window.playbackMatchesSelection", "Selected Library source reopened for playback.", "Selected Library source could not be reopened for playback.", "onClicked: window.reopenSelectedLibraryTrack()"):
        assert marker in QML, marker
    print("Issue #76 reopen selected result contract verified (5 checks)")
if __name__ == "__main__": main()
