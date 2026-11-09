#!/usr/bin/env python3
"""Portable contract for focusing Library recovery status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryRecoveryStatusFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+V"' in block
    assert "typeof storageRecovery !== \"undefined\"" in block
    assert "analysisRecoveryStatus.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisRecoveryStatus" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisRecoveryStatus"') - 80 :]
    assert "Ctrl+Alt+Shift+V recovery status" in SOURCE


if __name__ == "__main__":
    main()
