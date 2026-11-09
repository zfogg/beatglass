#!/usr/bin/env python3
"""Portable contract for the recursive return-button reset."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('id: returnToFolderBrowser')
    block = SOURCE[start : SOURCE.index('objectName: "recursiveResultsFrame"', start)]
    assert "window.recursiveCatalogActive = false" in block
    assert "recursiveResultsView.currentIndex = -1" in block
    assert 'search.text = ""' in block
    assert "browser.currentIndex = -1" in block
    assert "filter and entry selection were cleared" in block
    assert "browser.forceActiveFocus()" in block


if __name__ == "__main__":
    main()
