"""Contract checks for accessible state on Library playback actions."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    for object_name in ("openPlayback", "playSelectedTrack"):
        start = QML.index(f'objectName: "{object_name}"')
        block = QML[start : QML.index("\n", start)]
        assert "Accessible.value:" in block, object_name
        assert "unavailable; select an audio source" in block, object_name
        assert "unavailable; playback unavailable" in block, object_name
        assert "available; " in block, object_name
        assert "window.selectedName" in block, object_name

    start = QML.index('objectName: "reopenSelectedPlayback"')
    block = QML[start : QML.index("\n", start)]
    assert 'Accessible.value: "available; replace playback with " + window.selectedName' in block


if __name__ == "__main__":
    main()
