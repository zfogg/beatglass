"""Verify refreshing the Library clears stale browser selection."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'browser.currentIndex = -1' in QML
    assert 'entry selection cleared' in QML
    print("Issue #76 refresh selection contract verified")
if __name__ == "__main__": main()
