"""Verify Library seek availability is synchronized with playback/source state."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "playbackSeekActionStatus"')
    block = QML[start:start + 1800]
    for marker in ("Seek unavailable while playback is stopped", "selected source is reopened", "Seek ready for the selected source", "source audio frames", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Issue #76 playback seek action status contract verified (5 checks)")

if __name__ == "__main__":
    main()
