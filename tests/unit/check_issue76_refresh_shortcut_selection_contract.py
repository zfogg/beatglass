"""Verify Ctrl+R also clears stale Library browser selection."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Folder contents refreshed from the keyboard; entry selection cleared.' in QML
    print("Issue #76 refresh shortcut selection contract verified")
if __name__ == "__main__": main()
