"""Verify Reopen selected retains keyboard focus after recovery."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert "reopenSelectedPlayback.forceActiveFocus()" in QML
    print("Issue #76 reopen selected focus contract verified")
if __name__ == "__main__": main()
