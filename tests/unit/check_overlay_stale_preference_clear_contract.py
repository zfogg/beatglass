"""Executable contract for clearing stale display preferences."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("        if (!window.overlayBridge.moveToScreen", start)]
    assert "overlayPreferredScreenIndex = -1" in block
    assert 'overlayPreferredDisplayName = ""' in block
    assert "Overlay display selection is stale" in block
    print("Overlay stale preference clear contract verified (3 checks)")


if __name__ == "__main__":
    main()
