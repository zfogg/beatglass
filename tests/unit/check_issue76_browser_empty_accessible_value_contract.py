"""Verify the Library empty-state grouping exposes its current message."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: window.browserEmptyMessage()' in QML
    print("Issue #76 browser empty accessible value contract verified")
if __name__ == "__main__": main()
