"""Executable contract for stale display selection feedback."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("        if (!window.overlayBridge.moveToScreen", start)]
    assert "selection is stale" in block
    assert "choose an available display" in block
    assert "overlayDisplaySelectionRequired = true" in block
    print("Overlay stale selection announcement contract verified (3 checks)")


if __name__ == "__main__":
    main()
