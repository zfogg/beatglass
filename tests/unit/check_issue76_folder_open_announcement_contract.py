"""Verify opening a Library folder announces the destination."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Opened Library folder " + files.get(index, "fileName")' in QML
    print("Issue #76 folder open announcement contract verified")
if __name__ == "__main__": main()
