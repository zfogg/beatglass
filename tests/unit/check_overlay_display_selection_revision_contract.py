"""Executable contract for display selection settings synchronization."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("    function hideOverlayAndRestoreControls()", start)]
    restore = block.index("if (!window.restoreOverlayVisualStateFromBridge())")
    sync = block.index("window.syncOverlaySettingsStateFromBridge()")
    assert sync > restore
    assert sync < block.index("window.overlayPreferredScreenIndex = index")
    print("Overlay display selection revision contract verified (3 checks)")


if __name__ == "__main__":
    main()
