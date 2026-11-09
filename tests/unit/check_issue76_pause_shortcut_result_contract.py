"""Verify the pause shortcut uses the result-aware playback path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "playbackPauseShortcut"' in QML
    assert 'onActivated: window.togglePlaybackPause()' in QML
    print("Issue #76 pause shortcut result contract verified (2 checks)")
if __name__ == "__main__": main()
