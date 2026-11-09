"""Verify parent navigation clears the selected audio source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.clearSelectedAudio()\n                            window.browseFolder = files.parentFolder' in QML
    print("Issue #76 parent audio reset contract verified")
if __name__ == "__main__": main()
