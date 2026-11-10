"""Verify retrying a batch returns focus to live status."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'retryAnalysisBatchButton.forceActiveFocus() } analysisBatchStatus.forceActiveFocus()' in QML
    print("Issue #76 batch retry status focus contract verified")
if __name__ == "__main__": main()
