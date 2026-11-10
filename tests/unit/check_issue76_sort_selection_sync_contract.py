"""Verify reversing Library sort clears the selected row."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.sortDescending = !window.sortDescending\n        browser.currentIndex = -1' in QML
    print("Issue #76 sort selection sync contract verified")
if __name__ == "__main__": main()
