"""Source contract for native playback status accessibility."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for name in ("nativePlaybackState", "playbackState", "playbackSource", "playbackPosition"):
        start = QML.index(f'objectName: "{name}"')
        source = QML[start:QML.find("\n", start) + 1]
        assert "Accessible.value:" in source, name
        assert "Accessible.role: Accessible.Status" in source, name
    print("playback status accessibility contract: 4 statuses verified")

if __name__ == "__main__":
    main()
