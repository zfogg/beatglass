#!/usr/bin/env python3
"""Portable contract for Library folder transitions clearing stale selection."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("FolderDialog {")
    block = SOURCE[start:]
    assert "window.clearSelectedAudio()" in block
    assert "window.browseFolder = selectedFolder" in block
    assert "browser.currentIndex = -1" in block
    assert "window.recursiveCatalogActive = false" in block
    assert "browser.forceActiveFocus()" in block
    assert "Library folder selection canceled" in block


if __name__ == "__main__":
    main()
