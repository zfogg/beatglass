"""Verify the empty-state recovery button exposes its action state."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: "Recovery action available"' in QML
    print("Issue #76 empty recovery button value contract verified")
if __name__ == "__main__": main()
