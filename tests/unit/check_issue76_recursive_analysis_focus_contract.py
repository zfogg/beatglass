"""Verify recursive analysis submission restores focus to its results."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("submitSelectedFiles(paths)", "recursive audio files accepted for analysis.", "recursiveResultsView.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 recursive analysis focus contract verified (3 checks)")
if __name__ == "__main__": main()
