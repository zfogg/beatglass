"""Executable contract for persisted display restore failure feedback."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function reapplyOverlaySettingsAndRestore()")
    block = QML[start:QML.index("    function recoverOverlayAfterDisplayChange()", start)]
    for fragment in ("moveToScreen(window.overlayBridge.screenIndex)", "overlayDisplayRecoveryPending = true", "Persisted overlay display is unavailable", "choose another display"):
        assert fragment in block, f"missing contract fragment: {fragment}"
    print("Overlay display restore failure contract verified (4 checks)")


if __name__ == "__main__":
    main()
