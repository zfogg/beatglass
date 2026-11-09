#!/usr/bin/env python3
"""Portable contract for activating a focused Library browser entry."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryActivateEntryShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Enter"' in block
    assert "!window.recursiveCatalogActive" in block
    assert "browser.currentIndex >= 0" in block
    assert "window.activateEntry(browser.currentIndex)" in block
    assert "announceVisualizationRecovery" in block
    function = SOURCE[SOURCE.index("function activateEntry(index)") : SOURCE.index("function clearSelectedAudio")]
    assert "window.clearSelectedAudio()" in function
    assert "metadataBridge.inspect(url)" in function
    assert "analyzeSelectedAudio.forceActiveFocus()" in function


if __name__ == "__main__":
    main()
