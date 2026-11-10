"""Verify clearing the Library filter announces the visible count."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Library filter cleared; " + files.count + " folder entries are visible.' in QML
    print("Issue #76 clear filter count contract verified")
if __name__ == "__main__": main()
