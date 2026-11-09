"""Verify keyboard storage reopen is gated and announced."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"storageReopenShortcut\"", "sequence: \"Ctrl+Alt+O\"", "storageRecovery.open()", "Storage reopen requested from the keyboard"):
        assert marker in QML, marker
    print("Issue #76 storage reopen shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
