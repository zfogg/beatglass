#!/usr/bin/env python3
"""Portable contract for focusing Library analysis admission status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAdmissionStatusFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+X"' in block
    assert "analysisAdmissionStatus.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisAdmissionStatus" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisAdmissionStatus"') - 80 :]
    assert "Ctrl+Alt+Shift+X admission status" in SOURCE


if __name__ == "__main__":
    main()
