#!/usr/bin/env python3
"""Portable contract for focusing selected Library analysis action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+N"' in block
    assert 'window.selectedAudioUrl.toString() !== ""' in block
    assert "analyzeSelectedAudio.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "Ctrl+Alt+Shift+N analysis action" in SOURCE


if __name__ == "__main__":
    main()
