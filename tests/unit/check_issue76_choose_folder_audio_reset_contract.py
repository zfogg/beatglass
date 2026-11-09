"""Verify choosing a root clears the prior audio source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onAccepted: {\n            window.clearSelectedAudio()' in QML
    print("Issue #76 choose folder audio reset contract verified")
if __name__ == "__main__": main()
