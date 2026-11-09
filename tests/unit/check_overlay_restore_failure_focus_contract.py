"""Executable contract for focus restoration after overlay restore failures."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function reapplyOverlaySettingsAndRestore()")
    block = QML[start:QML.index("    function recoverOverlayAfterDisplayChange()", start)]
    assert block.count("overlayDisplay.forceActiveFocus()") >= 2
    assert "Persisted overlay display is unavailable" in block
    assert "restoreOverlayVisualStateFromBridge()" in block
    print("Overlay restore failure focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
