"""Verify the Library sort control exposes its current order."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: window.sortDescending ? "descending" : "ascending"' in QML
    print("Issue #76 sort accessible value contract verified")
if __name__ == "__main__": main()
