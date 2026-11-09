"""Verify keyboard bulk recovery discard is gated and announced."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"bulkRecoveryDiscardShortcut\"", "sequence: \"Ctrl+Alt+Shift+A\"", "storageRecovery.discardRuns(storageRecovery.retryableRunIds)", "All saved recovery runs discarded from the keyboard"):
        assert marker in QML, marker
    print("Issue #76 bulk recovery discard shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
