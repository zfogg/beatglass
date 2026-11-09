#!/usr/bin/env python3
"""Portable contract for focusing Library analysis source path."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisSourceFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+T"' in block
    assert "analysisSourcePath.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisSourcePath" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisSourcePath"') - 80 :]
    assert "Ctrl+Alt+Shift+T analysis source" in SOURCE


if __name__ == "__main__":
    main()
