#!/usr/bin/env python3
"""Portable contract for focusing Library batch source."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryBatchSourceFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+2"' in block
    assert "analysisBatchLastSource.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisBatchLastSource" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisBatchLastSource"') - 80 :]
    assert "Ctrl+Alt+Shift+2 batch source" in SOURCE


if __name__ == "__main__":
    main()
