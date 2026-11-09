"""Verify the visualization resume button shares the guarded pause path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "resumeVisualizationPlayback"' in QML
    assert 'onClicked: window.togglePlaybackPause()' in QML
    print("Issue #76 resume button shared contract verified (2 checks)")
if __name__ == "__main__": main()
