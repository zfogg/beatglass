"""Verify source changes clear stale visualization state before overlay reuse."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    marker = 'clearVisualizationForCaptureState("Selection cleared because the audio source changed; the overlay awaits fresh accepted evidence.")'
    assert QML.count(marker) >= 2
    assert "selectedAudioUrl = url" in QML
    assert "selectedAudioUrl = Qt.resolvedUrl(path)" in QML
    print("Overlay source-change clear contract verified (3 checks)")
if __name__ == "__main__": main()
