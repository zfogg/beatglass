#!/usr/bin/env python3
"""Portable contract for the Library sort-order keyboard shortcut."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"
SOURCE = QML.read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "librarySortShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start + 1)]
    assert 'sequence: "Ctrl+Alt+S"' in block
    assert "window.sortDescending = !window.sortDescending" in block
    assert "window.announceVisualizationRecovery" in block
    assert "sortOrder.forceActiveFocus()" in block
    assert 'objectName: "sortOrder"' in SOURCE


if __name__ == "__main__":
    main()
