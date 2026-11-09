#!/usr/bin/env python3
"""Portable contract for Library parent-folder keyboard navigation."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryParentFolderShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Up"' in block
    assert "files.parentFolder.toString()" in block
    assert "window.clearSelectedAudio()" in block
    assert "window.browseFolder = files.parentFolder" in block
    assert 'search.text = ""' in block
    assert "window.recursiveCatalogActive = false" in block
    assert "browser.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block


if __name__ == "__main__":
    main()
