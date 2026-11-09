"""Verify playback output refresh announces its recovery state."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("function refreshPlaybackOutputs()", "window.playbackBridge.refreshDevices()", "Playback outputs refreshed", "window.refreshPlaybackOutputs()"):
        assert marker in QML, marker
    print("Issue #76 playback output refresh contract verified (4 checks)")
if __name__ == "__main__": main()
