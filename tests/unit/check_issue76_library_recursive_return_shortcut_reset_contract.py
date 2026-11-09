#!/usr/bin/env python3
"""Portable contract for the Library recursive-return shortcut reset."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecursiveReturnShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+B"' in block
    assert "recursiveResultsView.currentIndex = -1" in block
    assert 'search.text = ""' in block
    assert "browser.currentIndex = -1" in block
    assert "filter, entry selection, and selected source were cleared" in block
    assert "browser.forceActiveFocus()" in block


if __name__ == "__main__":
    main()
