#!/usr/bin/env python3
"""Portable contract for focusing Library batch progress."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryBatchProgressFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+5"' in block
    assert "analysisBatchProgressBar.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisBatchProgressBar" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisBatchProgressBar"') - 80 :]
    assert "Ctrl+Alt+Shift+5 batch progress" in SOURCE


if __name__ == "__main__":
    main()
