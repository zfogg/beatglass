#!/usr/bin/env python3
"""Portable contract for keyboard parent navigation in the Library browser."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("Keys.onBackspacePressed")
    block = SOURCE[start : SOURCE.index("                        }", start)]
    assert "files.parentFolder.toString()" in block
    assert "window.clearSelectedAudio()" in block
    assert 'search.text = ""' in block
    assert "window.recursiveCatalogActive = false" in block
    assert "window.recursiveCatalogRows = []" in block
    assert "browser.currentIndex = -1" in block
    assert "event.accepted = true" in block
    assert "Already at the top Library folder" in block


if __name__ == "__main__":
    main()
