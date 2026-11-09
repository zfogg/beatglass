"""Source contract for keyboard-reachable output device selection."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "outputDevice"')
    source = QML[start:start + 700]
    assert "activeFocusOnTab: true" in source
    assert "Accessible.role: Accessible.ComboBox" in source
    assert "Output device" in source
    print("output device keyboard contract: verified")

if __name__ == "__main__":
    main()
