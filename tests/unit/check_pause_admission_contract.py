"""Source contract for click-time pause/resume admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "pausePlayback"')
    source = QML[start:start + 1200]
    assert 'state === "Playing"' in source
    assert 'state === "Paused"' in source
    assert "resume()" in source and "pause()" in source
    assert "no longer available" in source
    print("pause admission contract: verified")

if __name__ == "__main__":
    main()
