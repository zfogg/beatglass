#!/usr/bin/env python3
"""Portable contract for focusing the Library folder chooser."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryChooseFolderFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+D"' in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    choose = SOURCE[SOURCE.index('objectName: "chooseFolder"') - 80 :]
    assert "id: chooseFolder" in choose
    assert "Ctrl+Alt+Shift+D folder chooser" in SOURCE


if __name__ == "__main__":
    main()
