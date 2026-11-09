#!/usr/bin/env python3
"""Portable contract for focusing Library batch publication summary."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryBatchPublicationFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+3"' in block
    assert "analysisBatchPublicationSummary.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisBatchPublicationSummary" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisBatchPublicationSummary"') - 80 :]
    assert "Ctrl+Alt+Shift+3 publication summary" in SOURCE


if __name__ == "__main__":
    main()
