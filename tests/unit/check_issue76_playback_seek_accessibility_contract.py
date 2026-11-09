"""Verify Library playback seek controls expose stable accessible names."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    assert 'objectName: "seekBackPlayback"' in QML
    assert 'seekSeconds.value === 5 ? "Seek backward five seconds"' in QML
    assert 'objectName: "seekForwardPlayback"' in QML
    assert 'seekSeconds.value === 5 ? "Seek forward five seconds"' in QML
    assert 'Accessible.role: Accessible.Button' in QML[QML.index('objectName: "seekBackPlayback"'):QML.index('objectName: "seekBackPlayback"') + 600]
    print("Issue #76 playback seek accessibility contract verified (5 checks)")

if __name__ == "__main__":
    main()
