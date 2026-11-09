#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on admitted cancel."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "cancelAdmittedAnalysis"')
    block = SOURCE[start : SOURCE.index('objectName: "retryAdmittedAnalysis"', start)]
    assert "storageRecovery.cancelAnalysis()" in block
    assert "window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until a replacement accepted snapshot" in block


if __name__ == "__main__":
    main()
