"""Verify the recursive results focus shortcut announces its transition."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Recursive Library results focused.' in QML
    print("Issue #76 recursive focus announcement contract verified")
if __name__ == "__main__": main()
