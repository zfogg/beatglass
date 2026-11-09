"""Source contract for output endpoint empty-state accessibility."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "endpointEmptyState"')
    source = QML[start:start + 700]
    assert "Accessible.value: text" in source
    assert "Accessible.role: Accessible.Status" in source
    assert "Refresh after connecting an output device" in source
    print("endpoint empty-state accessibility contract: verified")

if __name__ == "__main__":
    main()
