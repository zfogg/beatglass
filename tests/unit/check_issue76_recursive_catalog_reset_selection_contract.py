"""Verify recursive cataloging clears the selected audio source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onClicked: {\n                            window.clearSelectedAudio()\n                            window.recursiveCatalogRows' in QML
    print("Issue #76 recursive catalog selection reset contract verified")
if __name__ == "__main__": main()
