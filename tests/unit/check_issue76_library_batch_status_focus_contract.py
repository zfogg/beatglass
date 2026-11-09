#!/usr/bin/env python3
"""Portable contract for focusing Library analysis batch status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryBatchStatusFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+U"' in block
    assert "analysisBatchStatus.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    status = SOURCE[SOURCE.index('objectName: "analysisBatchStatus"') - 80 :]
    assert "id: analysisBatchStatus" in status
    assert "activeFocusOnTab: true" in status
    assert "Ctrl+Alt+Shift+U batch status" in SOURCE


if __name__ == "__main__":
    main()
