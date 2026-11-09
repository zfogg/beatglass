"""Verify overlay keyboard shortcuts are discoverable in the control window."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayShortcutHelp"')
    block = QML[start:start + 700]
    for marker in ("Esc unlocks", "Ctrl+Alt+I toggles click-through", "Ctrl+Alt+D focuses display", "Ctrl+Alt+Y retries overlay", "Ctrl+Alt+O cycles size", "Ctrl+Alt+Shift+O resets opacity", "Ctrl+Alt+Shift+R reapplies saved settings", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay shortcut help contract verified (4 checks)")

if __name__ == "__main__":
    main()
