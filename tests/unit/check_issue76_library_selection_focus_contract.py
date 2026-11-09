#!/usr/bin/env python3
"""Portable contract for focusing Library selection status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "librarySelectionStatusFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+E"' in block
    assert "librarySelectionStatus.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    status = SOURCE[SOURCE.index('objectName: "librarySelectionStatus"') - 80 :]
    assert "id: librarySelectionStatus" in status
    assert "activeFocusOnTab: true" in status
    assert "Ctrl+Alt+Shift+E selection status" in SOURCE


if __name__ == "__main__":
    main()
