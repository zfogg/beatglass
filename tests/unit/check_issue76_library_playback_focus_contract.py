#!/usr/bin/env python3
"""Portable contract for focusing selected Library playback action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryPlaybackFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+Y"' in block
    assert 'window.selectedAudioUrl.toString() !== ""' in block
    assert "playSelectedTrack.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "Ctrl+Alt+Shift+Y playback action" in SOURCE


if __name__ == "__main__":
    main()
