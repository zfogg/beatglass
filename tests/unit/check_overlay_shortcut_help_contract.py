"""Verify overlay keyboard shortcuts are discoverable in the control window."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayShortcutHelp"')
    block = QML[start:start + 1500]
    for marker in ("Esc unlocks", "Ctrl+Alt+U focuses unlock", "Ctrl+Alt+I toggles click-through", "Ctrl+Alt+H hides/restores", "Ctrl+Alt+J focuses hide", "Ctrl+Alt+W focuses show", "Ctrl+Alt+Shift+W focuses restore", "Ctrl+Alt+D focuses display", "Ctrl+Alt+Y retries overlay", "Ctrl+Alt+Shift+Y focuses retry", "Ctrl+Alt+O cycles size", "Ctrl+Alt+Z focuses size", "Ctrl+Alt+Shift+O resets opacity", "Ctrl+Alt+L focuses opacity", "Ctrl+Alt+Shift+R reapplies saved settings", "Ctrl+Alt+Shift+P focuses reapply", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay shortcut help contract verified (4 checks)")

if __name__ == "__main__":
    main()
