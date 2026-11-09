#!/usr/bin/env python3
"""Portable contract for focusing recursive Library return action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecursiveReturnFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+J"' in block
    assert "window.recursiveCatalogActive" in block
    assert "returnToFolderBrowser.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: returnToFolderBrowser" in SOURCE
    assert "Ctrl+Alt+Shift+J return action" in SOURCE


if __name__ == "__main__":
    main()
