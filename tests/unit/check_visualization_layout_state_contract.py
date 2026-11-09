"""Verify compact/expanded visualization layout is exposed accessibly."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "visualizationLayoutState"')
    block = QML[start:start + 700]
    for marker in ("Compact visualization layout", "Expanded visualization layout", "Accessible.role: Accessible.Status", "window.compactHeader"):
        assert marker in block, marker
    print("Visualization layout state contract verified (4 checks)")

if __name__ == "__main__":
    main()
