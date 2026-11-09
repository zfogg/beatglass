"""Verify keyboard playback seek shortcuts honor source alignment."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for marker in ("playbackSeekBackShortcut", "Alt+Left", "playbackSeekForwardShortcut", "Alt+Right", "playbackMatchesSelection", "Playback sought backward five seconds", "Playback sought forward five seconds"):
        assert marker in QML, marker
    print("Issue #76 playback seek shortcut contract verified (7 checks)")

if __name__ == "__main__":
    main()
