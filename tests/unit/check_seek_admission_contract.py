"""Source contract for click-time seek admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "seekToFrame"')
    source = QML[start:start + 900]
    assert 'state !== "Stopped"' in source
    assert "seek(seekFrame.value" in source
    assert "Seek is no longer available" in source
    print("seek admission contract: verified")

if __name__ == "__main__":
    main()
