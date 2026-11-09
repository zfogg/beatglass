#!/usr/bin/env python3
"""Portable contract for Library analysis retry focus."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryAnalysis"')
    line = SOURCE[start - 180 : SOURCE.index("\n", start)]
    assert "id: retryAnalysis" in line
    assert "window.retryVisualizationAnalysis()" in SOURCE
    assert "retryAnalysis.forceActiveFocus()" in SOURCE
    function = SOURCE[SOURCE.index("function retryVisualizationAnalysis") :]
    assert "announceVisualizationRecovery" in function
    assert "canRetry" in SOURCE[SOURCE.index('objectName: "retryAnalysis"') - 260 : SOURCE.index('objectName: "retryAnalysis"') + 200]


if __name__ == "__main__":
    main()
