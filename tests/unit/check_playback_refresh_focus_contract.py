"""Source contract for playback output refresh focus restoration."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "refreshPlaybackDevices"')
    source = QML[start:start + 800]
    assert "refreshDevices()" in source
    assert "playbackDevice.forceActiveFocus()" in source
    print("playback refresh focus contract: verified")

if __name__ == "__main__":
    main()
