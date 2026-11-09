"""Verify folder-browser items participate in keyboard focus traversal."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert "required property string fileName" in QML
    assert "activeFocusOnTab: true" in QML
    print("Issue #76 browser item focus contract verified")
if __name__ == "__main__": main()
