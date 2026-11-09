#!/usr/bin/env python3
"""Portable contract for clearing the Library filter."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryClearFilterShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+Backspace"' in block
    assert 'enabled: search.text !== ""' in block
    assert 'search.text = ""' in block
    assert "browser.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "Ctrl+Alt+Shift+Backspace clear filter" in SOURCE


if __name__ == "__main__":
    main()
