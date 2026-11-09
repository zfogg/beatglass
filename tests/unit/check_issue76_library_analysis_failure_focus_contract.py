#!/usr/bin/env python3
"""Portable contract for focusing Library analysis failure status."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "libraryAnalysisFailureFocusShortcut"')
    block = SOURCE[start : SOURCE.index("    Shortcut {", start)]
    assert 'sequence: "Ctrl+Alt+Shift+7"' in block
    assert "analysisFailureReason.forceActiveFocus()" in block
    assert "announceVisualizationRecovery" in block
    assert "id: analysisFailureReason" in SOURCE
    assert "activeFocusOnTab: true" in SOURCE[SOURCE.index('objectName: "analysisFailureReason"') - 80 :]
    assert "Ctrl+Alt+Shift+7 analysis failure" in SOURCE


if __name__ == "__main__":
    main()
