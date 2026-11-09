#!/usr/bin/env python3
"""Portable contract for focusing Library analysis freshness."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisFreshnessFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+0"' in block
    assert "analysisFreshness.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisFreshness" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisFreshness"') - 80 :]
    assert "Ctrl+Alt+Shift+0 analysis freshness" in SOURCE


if __name__ == "__main__":
    main()
