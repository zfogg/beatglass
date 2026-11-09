#!/usr/bin/env python3
"""Portable contract for focusing the current Library folder path."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryFolderPathFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+I"' in block
    assert "folderPath.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    path = SOURCE[SOURCE.index('objectName: "folderPath"') - 80 :]
    assert "id: folderPath" in path
    assert "activeFocusOnTab: true" in path
    assert "Ctrl+Alt+Shift+I folder path" in SOURCE


if __name__ == "__main__":
    main()
