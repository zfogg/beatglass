"""Verify the stop button shares the guarded playback stop path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "stopPlayback"' in QML
    assert 'onClicked: window.stopPlaybackWithRecovery("Playback stopped;' in QML
    print("Issue #76 stop button contract verified (2 checks)")
if __name__ == "__main__": main()
