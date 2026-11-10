"""Verify recursive results expose their count accessibly."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: window.recursiveCatalogRows.length + " results"' in QML
    print("Issue #76 recursive results count accessibility contract verified")
if __name__ == "__main__": main()
