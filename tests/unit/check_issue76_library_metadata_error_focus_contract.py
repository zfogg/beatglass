#!/usr/bin/env python3
"""Portable contract for focusing Library metadata error guidance."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryMetadataErrorFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+F"' in block
    assert 'window.metadataStatus.indexOf("Error:") === 0' in block
    assert "metadataErrorGuidance.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: metadataErrorGuidance" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "metadataErrorGuidance"') - 80 :]
    assert "Ctrl+Alt+Shift+F metadata error" in SOURCE


if __name__ == "__main__":
    main()
