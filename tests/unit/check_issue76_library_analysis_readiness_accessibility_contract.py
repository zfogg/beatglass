#!/usr/bin/env python3
"""Portable contract for Library analysis readiness messaging."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "detailsAvailability"')
    block = SOURCE[start : SOURCE.index('objectName: "analysisUncertainty"', start)]
    assert "libraryAnalysisResultReady" in block
    assert "source-linked and ready" in block
    assert "provisional" in block


if __name__ == "__main__":
    main()
