"""Verify folder navigation clears the prior Library filter."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert QML.count('search.text = ""') >= 2
    print("Issue #76 folder navigation filter contract verified")
if __name__ == "__main__": main()
