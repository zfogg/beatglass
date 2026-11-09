#!/usr/bin/env python3
"""Portable contract for visible Library shortcut help."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryShortcutHelp"')
    block = SOURCE[start : SOURCE.index("                }", start)]
    for shortcut in ("Ctrl+F", "Ctrl+Alt+L", "Ctrl+Alt+Shift+L", "Ctrl+Alt+S", "Ctrl+Alt+Backspace"):
        assert shortcut in block
    assert 'Accessible.role: Accessible.StaticText' in block
    assert "Keyboard shortcuts for filtering" in block
    tab = SOURCE[SOURCE.index('objectName: "libraryTabShortcut"') :]
    assert "Library tab focused." in tab


if __name__ == "__main__":
    main()
