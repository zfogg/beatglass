#!/usr/bin/env python3
"""Portable contract for focusing Library recursive catalog action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecursiveFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+Z"' in block
    assert "!window.recursiveCatalogActive" in block
    assert "addFolderRecursively.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "Ctrl+Alt+Shift+Z recursive action" in SOURCE


if __name__ == "__main__":
    main()
