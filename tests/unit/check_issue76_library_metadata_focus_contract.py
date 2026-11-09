#!/usr/bin/env python3
"""Portable contract for focusing selected Library metadata status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryMetadataFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+M"' in block
    assert 'window.selectedAudioUrl.toString() !== ""' in block
    assert "metadataStatusLabel.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    status = SOURCE[SOURCE.index('objectName: "metadataStatus"') - 120 :]
    assert "id: metadataStatusLabel" in status
    assert "activeFocusOnTab: true" in status
    assert "Ctrl+Alt+Shift+M metadata status" in SOURCE


if __name__ == "__main__":
    main()
