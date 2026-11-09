"""Verify opening a folder clears selected audio."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'if (files.isFolder(index)) {\n            window.clearSelectedAudio()' in QML
    print("Issue #76 folder open audio reset contract verified")
if __name__ == "__main__": main()
