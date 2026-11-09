"""Verify stop shortcut uses a shared recovery path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("function stopPlaybackWithRecovery(message)", "window.stopPlaybackWithRecovery(\"Playback stopped from keyboard shortcut", "window.playbackBridge.stop()"):
        assert marker in QML, marker
    print("Issue #76 stop shortcut contract verified (3 checks)")
if __name__ == "__main__": main()
