#!/usr/bin/env python3
"""Portable contract for the recursive Library catalog shortcut."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecursiveCatalogShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+R"' in block
    assert "window.browseFolder.toString()" in block
    assert "!window.recursiveCatalogActive" in block
    assert "addFolderRecursively.click()" in block
    assert "announceVisualizationRecovery" in block
    assert "window.recursiveCatalogRows = window.libraryCatalogBridge.browseRecursive" in SOURCE


if __name__ == "__main__":
    main()
