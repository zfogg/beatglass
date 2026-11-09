#!/usr/bin/env python3
"""Portable contract for focusing Library batch result."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryBatchResultFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+4"' in block
    assert "analysisBatchLastResult.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisBatchLastResult" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisBatchLastResult"') - 80 :]
    assert "Ctrl+Alt+Shift+4 batch result" in SOURCE


if __name__ == "__main__":
    main()
