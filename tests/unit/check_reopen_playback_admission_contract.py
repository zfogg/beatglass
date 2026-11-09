"""Source contract for click-time playback source admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "reopenSelectedPlayback"')
    source = QML[start:start + 1200]
    assert "playbackMatchesSelection" in source
    assert "openSource(" in source
    assert "no longer needed" in source
    print("reopen playback admission contract: verified")

if __name__ == "__main__":
    main()
