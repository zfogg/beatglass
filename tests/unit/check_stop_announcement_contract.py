"""Source contract for successful stop announcement."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "stopPlayback"')
    source = QML[start:start + 1100]
    assert "window.playbackBridge.stop()" in source
    assert "Playback stopped; the source is ready to reopen" in source
    assert "already stopped" in source
    print("stop announcement contract: verified")

if __name__ == "__main__":
    main()
