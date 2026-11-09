"""Verify parent navigation closes stale recursive results."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.recursiveCatalogActive = false\n                            window.recursiveCatalogRows = []' in QML
    print("Issue #76 parent recursive reset contract verified")
if __name__ == "__main__": main()
