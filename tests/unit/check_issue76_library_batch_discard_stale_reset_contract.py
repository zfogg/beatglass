#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on batch discard."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "discardAnalysisBatchDialog"')
    block = SOURCE[start : start + 1600]
    assert "window.analysisJobBridge.discardBatch()" in block
    assert "window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until a new accepted result" in block


if __name__ == "__main__":
    main()
