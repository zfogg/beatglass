"""Verify recursive batch analysis clears stale selected audio."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onClicked: {\n                            window.clearSelectedAudio()\n                            const paths = []' in QML
    print("Issue #76 recursive batch reset contract verified")
if __name__ == "__main__": main()
