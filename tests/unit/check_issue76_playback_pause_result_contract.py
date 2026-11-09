"""Verify pause/resume control reports the bridge result."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("function togglePlaybackPause()", "const changed = paused ? window.playbackBridge.resume() : window.playbackBridge.pause()", "Playback pause or resume failed", "onClicked: window.togglePlaybackPause()"):
        assert marker in QML, marker
    print("Issue #76 playback pause result contract verified (4 checks)")
if __name__ == "__main__": main()
