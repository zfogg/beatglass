"""Verify recursive batch accessibility text states its bound."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'queue up to two of the first available recursive audio results' in QML
    print("Issue #76 recursive batch bound accessibility contract verified")
if __name__ == "__main__": main()
