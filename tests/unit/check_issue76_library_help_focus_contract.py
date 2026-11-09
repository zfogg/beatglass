#!/usr/bin/env python3
"""Portable contract for focusing Library keyboard shortcut help."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryShortcutHelpFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+H"' in block
    assert "libraryShortcutHelp.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    help_block = SOURCE[SOURCE.index('objectName: "libraryShortcutHelp"') - 80 :]
    assert "id: libraryShortcutHelp" in help_block
    assert "activeFocusOnTab: true" in help_block
    assert "Ctrl+Alt+Shift+H help" in SOURCE


if __name__ == "__main__":
    main()
