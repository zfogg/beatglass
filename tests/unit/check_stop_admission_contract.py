"""Source contract for click-time stop admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "stopPlayback"')
    source = QML[start:start + 900]
    assert 'state !== "Stopped"' in source
    assert "stop()" in source
    assert "no longer needed" in source
    print("stop admission contract: verified")

if __name__ == "__main__":
    main()
