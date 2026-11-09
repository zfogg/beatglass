#!/usr/bin/env python3
"""Portable contract for Library analysis freshness guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisFreshness"')
    block = SOURCE[start : SOURCE.index('objectName: "analysisProvenance"', start)]
    assert "source-linked presentation" in block
    assert "Wait for accepted source-linked analysis" in block


if __name__ == "__main__":
    main()
