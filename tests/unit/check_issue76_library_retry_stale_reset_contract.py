#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on analysis retry."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("function retryVisualizationAnalysis()")
    block = SOURCE[start : SOURCE.index("    Connections", start)]
    assert "window.analysisJobBridge.retry()" in block
    assert "storageRecovery.retryAnalysis()" in block
    assert block.count("clearVisualizationForCaptureState") == 2
    assert "previous visualization evidence is stale until completion" in block


if __name__ == "__main__":
    main()
