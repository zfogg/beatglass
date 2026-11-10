"""Verify recursive batch announcements include rejected count."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'recursive audio files accepted for analysis; " + window.analysisJobBridge.batchRejectedCount + " rejected.' in QML
    print("Issue #76 recursive batch announcement contract verified")
if __name__ == "__main__": main()
