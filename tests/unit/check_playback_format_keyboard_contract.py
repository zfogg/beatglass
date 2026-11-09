"""Source contract for keyboard-reachable playback format controls."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for name in ("playbackSampleRate", "playbackChannels"):
        start = QML.index(f'objectName: "{name}"')
        source = QML[start:start + 600]
        assert "activeFocusOnTab: true" in source, name
        assert "Accessible.role: Accessible.SpinBox" in source, name
    start = QML.index('objectName: "startNativePlayback"')
    assert "playbackDevice.currentIndex >= 0" in QML[start:start + 700]
    print("playback format keyboard contract: 2 controls verified")

if __name__ == "__main__":
    main()
