#!/usr/bin/env python3
"""Portable contract for focusing Library recovery next action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecoveryNextActionFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+1"' in block
    assert "analysisRecoveryNextAction.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisRecoveryNextAction" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisRecoveryNextAction"') - 80 :]
    assert "Ctrl+Alt+Shift+1 recovery action" in SOURCE


if __name__ == "__main__":
    main()
