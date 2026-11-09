"""Verify visualization stop shares the guarded stop path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "stopVisualizationPlayback"' in QML
    assert 'onClicked: window.stopPlaybackWithRecovery("Playback stopped; accepted snapshot cleared.")' in QML
    print("Issue #76 visualization stop shared contract verified (2 checks)")
if __name__ == "__main__": main()
