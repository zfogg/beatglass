"""Verify recursive batch submission focuses live queue status."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'analysisBatchStatus.forceActiveFocus()' in QML
    print("Issue #76 recursive batch status focus contract verified")
if __name__ == "__main__": main()
