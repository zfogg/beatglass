"""Verify the keyboard shortcut invokes persisted overlay restoration."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayReapplyShortcut"')
    block = QML[start:start + 1000]
    for marker in ('sequence: "Ctrl+Alt+Shift+R"', "reapplyOverlaySettingsAndRestore", "saved overlay display, size, position, and opacity"):
        assert marker in block, marker
    print("Overlay reapply shortcut contract verified (3 checks)")

if __name__ == "__main__":
    main()
