"""Verify the Library has a direct selected-track playback action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "playSelectedTrack"')
    block = QML[start:start + 1300]
    for marker in ("Play selected Library track", "openSource", "Playing selected Library source", "Accessible.role: Accessible.Button", "playSelectedTrack.forceActiveFocus()"):
        assert marker in block, marker
    assert 'window.playbackBridge.epoch + 1' in QML[QML.index('objectName: "openPlayback"'):QML.index('objectName: "openPlayback"') + 800]
    print("Issue #76 selected-track playback action contract verified (6 checks)")

if __name__ == "__main__":
    main()
