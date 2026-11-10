"""Verify recursive Library rows expose their item type."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: folder ? "folder" : "audio file"' in QML
    print("Issue #76 recursive result accessible value contract verified")
if __name__ == "__main__": main()
