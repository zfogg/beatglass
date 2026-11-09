"""Executable contract for native unlock failure handling."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function unlockOverlayAndRestoreFocus()")
    block = QML[start:QML.index("    function reapplyOverlaySettingsAndRestore()", start)]
    for fragment in ("if (!window.overlayBridge.setClickThrough(false))", "could not be unlocked", "return false", "DJ overlay unlocked"):
        assert fragment in block, f"missing contract fragment: {fragment}"
    print("Overlay unlock failure contract verified (4 checks)")


if __name__ == "__main__":
    main()
