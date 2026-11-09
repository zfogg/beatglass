"""Verify keyboard activation works in recursive Library results."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Keys.onReturnPressed: { if (currentIndex >= 0) currentItem.click(); event.accepted = true }' in QML
    assert 'Keys.onEnterPressed:' in QML and 'Keys.onSpacePressed:' in QML
    print("Issue #76 recursive keyboard activation contract verified")
if __name__ == "__main__": main()
