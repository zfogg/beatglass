#!/usr/bin/env python3
"""Portable contract for Library analysis uncertainty guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisUncertainty"')
    block = SOURCE[start : SOURCE.index('objectName: "analysisFreshness"', start)]
    assert "provisional" in block
    assert "source-linked analysis is accepted" in block


if __name__ == "__main__":
    main()
