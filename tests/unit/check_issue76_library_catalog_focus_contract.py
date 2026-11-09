#!/usr/bin/env python3
"""Portable contract for focusing Library catalog status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryCatalogStatusFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+C"' in block
    assert "catalogResultCount.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    label = SOURCE[SOURCE.index('objectName: "catalogResultCount"') - 80 :]
    assert "id: catalogResultCount" in label
    assert "activeFocusOnTab: true" in label
    assert "Ctrl+Alt+Shift+C catalog status" in SOURCE


if __name__ == "__main__":
    main()
