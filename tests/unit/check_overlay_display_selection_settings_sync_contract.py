"""Executable contract for display selection settings synchronization."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("    function hideOverlayAndRestoreControls()", start)]
    assert "window.syncOverlaySettingsStateFromBridge()" in block
    assert block.index("window.syncOverlaySettingsStateFromBridge()") < block.index("window.overlayPreferredScreenIndex = index")
    print("Overlay display selection settings sync contract verified (2 checks)")


if __name__ == "__main__":
    main()
