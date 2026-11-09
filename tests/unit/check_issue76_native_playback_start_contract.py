"""Verify native playback start reports its actual result."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("function startNativePlaybackOutput()", "const started = window.playbackBridge.startNative", "Native playback started", "Native playback could not start", "onClicked: window.startNativePlaybackOutput()"):
        assert marker in QML, marker
    print("Issue #76 native playback start contract verified (5 checks)")
if __name__ == "__main__": main()
