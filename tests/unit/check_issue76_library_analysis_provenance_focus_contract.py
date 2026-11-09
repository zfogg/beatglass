#!/usr/bin/env python3
"""Portable contract for focusing Library analysis provenance."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisProvenanceFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+9"' in block
    assert "analysisProvenance.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisProvenance" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisProvenance"') - 80 :]
    assert "Ctrl+Alt+Shift+9 analysis provenance" in SOURCE


if __name__ == "__main__":
    main()
