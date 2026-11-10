"""Verify the Library refresh action exposes availability."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: enabled ? "available" : "choose a folder first"' in QML
    print("Issue #76 refresh action accessible value contract verified")
if __name__ == "__main__": main()
