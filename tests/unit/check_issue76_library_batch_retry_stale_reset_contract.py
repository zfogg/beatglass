#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on batch retry."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryAnalysisBatch"')
    block = SOURCE[start : SOURCE.index('objectName: "discardAnalysisBatch"', start)]
    assert "const accepted = window.analysisJobBridge.retryBatch()" in block
    assert "if (accepted) window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until completion" in block


if __name__ == "__main__":
    main()
