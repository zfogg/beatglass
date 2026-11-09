"""Executable contract for invalid persisted visual state recovery."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    function_start = QML.index("function reapplyOverlaySettingsAndRestore()")
    start = QML.index("if (!window.restoreOverlayVisualStateFromBridge())", function_start)
    block = QML[start:QML.index("        if (window.overlayBridge", start)]
    assert "overlayDisplayRecoveryPending = true" in block
    assert "Persisted overlay visual state is unavailable" in block
    assert "choose a display" in block
    print("Overlay visual restore failure contract verified (3 checks)")


if __name__ == "__main__":
    main()
