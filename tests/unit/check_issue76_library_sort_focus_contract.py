#!/usr/bin/env python3
"""Portable contract for focusing the Library sort control."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "librarySortFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+O"' in block
    assert "navigation.currentIndex = 0" in block
    assert "sortOrder.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    help_start = SOURCE.index('objectName: "libraryShortcutHelp"')
    help_block = SOURCE[help_start : SOURCE.index("                }", help_start)]
    assert "Ctrl+Alt+Shift+O sort control" in help_block


if __name__ == "__main__":
    main()
