"""Source contract for overlay display recovery statuses."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for name in ("overlayDisplayEmptyState", "overlayMissingDisplayRecovery"):
        start = QML.index(f'objectName: "{name}"')
        source = QML[start:start + 700]
        assert "Accessible.value:" in source, name
        assert "Accessible.role: Accessible.Status" in source, name
    print("overlay display recovery contract: 2 statuses verified")

if __name__ == "__main__":
    main()
