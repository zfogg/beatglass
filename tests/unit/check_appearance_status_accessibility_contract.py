"""Source contract for appearance and settings status accessibility."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for name in ("appearanceState", "settingsStatus"):
        start = QML.index(f'objectName: "{name}"')
        source = QML[start:QML.find("\n                        }", start)]
        assert "Accessible.value:" in source, name
        assert "Accessible.role: Accessible.Status" in source, name
    assert 'Accessible.name: "Appearance guidance"' in QML
    assert 'Accessible.role: Accessible.Status' in QML[QML.index('Accessible.name: "Appearance guidance"'):]
    print("appearance status accessibility contract: 3 statuses verified")

if __name__ == "__main__":
    main()
