#!/usr/bin/env python3
"""Portable contract for focusing Library analysis queue progress."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisQueueFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+6"' in block
    assert "analysisQueueProgress.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisQueueProgress" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisQueueProgress"') - 80 :]
    assert "Ctrl+Alt+Shift+6 queue progress" in SOURCE


if __name__ == "__main__":
    main()
