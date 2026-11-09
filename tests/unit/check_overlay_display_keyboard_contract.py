"""Source contract for keyboard-reachable overlay display selection."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayDisplay"')
    source = QML[start:start + 700]
    assert "activeFocusOnTab: true" in source
    assert "Accessible.role: Accessible.ComboBox" in source
    assert "persistOverlayDisplaySelection" in QML[start:start + 1400]
    print("overlay display keyboard contract: verified")

if __name__ == "__main__":
    main()
