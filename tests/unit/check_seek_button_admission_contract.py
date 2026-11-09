"""Source contract for click-time seek button admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for name, phrase in (("seekBackPlayback", "Seek backward is no longer available"), ("seekForwardPlayback", "Seek forward is no longer available")):
        start = QML.index(f'objectName: "{name}"')
        source = QML[start:start + 900]
        assert 'state !== "Stopped"' in source, name
        assert "seek(" in source, name
        assert phrase in source, name
    print("seek button admission contract: 2 controls verified")

if __name__ == "__main__":
    main()
