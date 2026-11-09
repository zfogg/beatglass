#!/usr/bin/env python3
"""Portable contract for focusing the Library refresh action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRefreshFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+Q"' in block
    assert 'window.browseFolder.toString() !== ""' in block
    assert "refreshFolder.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    refresh = SOURCE[SOURCE.index('objectName: "refreshFolder"') - 80 :]
    assert "id: refreshFolder" in refresh
    assert "Ctrl+Alt+Shift+Q refresh action" in SOURCE


if __name__ == "__main__":
    main()
