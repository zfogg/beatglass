"""Verify catalog search reports matches and returns focus to the browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("onAccepted: { window.announceVisualizationRecovery(files.count + \" Library entries match the filter.\")", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 catalog search focus contract verified (2 checks)")
if __name__ == "__main__": main()
