"""Verify the Library parent action exposes availability."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: enabled ? "available" : "at folder root"' in QML
    print("Issue #76 parent action accessible value contract verified")
if __name__ == "__main__": main()
