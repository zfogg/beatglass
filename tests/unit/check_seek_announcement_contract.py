"""Source contract for successful seek announcement."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "seekToFrame"')
    source = QML[start:start + 1200]
    assert "seek(seekFrame.value" in source
    assert "Playback seek requested to frame" in source
    assert "Seek is no longer available" in source
    print("seek announcement contract: verified")

if __name__ == "__main__":
    main()
