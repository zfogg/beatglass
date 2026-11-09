#!/usr/bin/env python3
"""Portable contract for focusing Library parent-folder action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryParentFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+W"' in block
    assert "files.parentFolder.toString()" in block
    assert "parentFolder.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    action = SOURCE[SOURCE.index('objectName: "parentFolder"') - 80 :]
    assert "id: parentFolder" in action
    assert "Ctrl+Alt+Shift+W parent action" in SOURCE


if __name__ == "__main__":
    main()
