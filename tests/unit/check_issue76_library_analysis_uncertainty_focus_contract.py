#!/usr/bin/env python3
"""Portable contract for focusing Library analysis uncertainty."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisUncertaintyFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+8"' in block
    assert "analysisUncertainty.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisUncertainty" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisUncertainty"') - 80 :]
    assert "Ctrl+Alt+Shift+8 analysis uncertainty" in SOURCE


if __name__ == "__main__":
    main()
