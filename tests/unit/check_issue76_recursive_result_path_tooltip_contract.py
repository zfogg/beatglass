"""Verify recursive rows expose their source path in a tooltip."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'ToolTip.text: path' in QML
    print("Issue #76 recursive result path tooltip contract verified")
if __name__ == "__main__": main()
