"""Source contract for native playback start announcement."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "startNativePlayback"')
    source = QML[start:start + 1300]
    assert "startNative(" in source
    assert "Native playback start requested" in source
    assert "playbackDevice.currentIndex >= 0" in source
    print("start native announcement contract: verified")

if __name__ == "__main__":
    main()
