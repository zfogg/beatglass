"""Verify the Library folder chooser opens at the current folder."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'currentFolder: window.browseFolder' in QML
    print("Issue #76 folder dialog location contract verified")
if __name__ == "__main__": main()
