#!/usr/bin/env python3
"""Portable contract for Library keyboard focus announcements."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def shortcut(name: str) -> str:
    start = SOURCE.index('objectName: "' + name + '"')
    end = SOURCE.index("    Shortcut {", start + 1)
    return SOURCE[start:end]


def main() -> None:
    assert "search.forceActiveFocus()" in shortcut("librarySearchShortcut")
    assert "announceVisualizationRecovery" in shortcut("librarySearchShortcut")
    assert "browser.forceActiveFocus()" in shortcut("libraryBrowserShortcut")
    assert "announceVisualizationRecovery" in shortcut("libraryBrowserShortcut")
    results = shortcut("libraryResultsShortcut")
    assert "recursiveResultsView.forceActiveFocus()" in results
    assert "browser.forceActiveFocus()" in results
    assert "announceVisualizationRecovery" in results


if __name__ == "__main__":
    main()
