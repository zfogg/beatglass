#!/usr/bin/env python3
"""Portable contract for focusing Library analysis progress."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisProgressFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+G"' in block
    assert "analysisProgress.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisProgress" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisProgress"') - 80 :]
    assert "Ctrl+Alt+Shift+G analysis progress" in SOURCE


if __name__ == "__main__":
    main()
