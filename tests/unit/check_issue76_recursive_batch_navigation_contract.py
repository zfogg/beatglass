"""Verify recursive batch analysis selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onClicked: {\n                            navigation.currentIndex = 0\n                            window.clearSelectedAudio()' in QML
    print("Issue #76 recursive batch navigation contract verified")
if __name__ == "__main__": main()
