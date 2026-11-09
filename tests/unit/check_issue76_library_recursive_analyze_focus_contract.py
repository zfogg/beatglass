#!/usr/bin/env python3
"""Portable contract for focusing recursive Library analysis action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecursiveAnalyzeFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+K"' in block
    assert "window.recursiveCatalogActive" in block
    assert "analyzeRecursiveResults.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analyzeRecursiveResults" in SOURCE
    assert "Ctrl+Alt+Shift+K recursive analysis" in SOURCE


if __name__ == "__main__":
    main()
