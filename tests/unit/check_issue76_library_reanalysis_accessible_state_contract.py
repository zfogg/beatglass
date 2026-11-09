#!/usr/bin/env python3
"""Portable contract for Library reanalysis accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "reanalyzeInvalidatedCache"')
    block = SOURCE[start : SOURCE.index('objectName: "reanalyzeCacheDialog"', start)]
    assert "Accessible.value: enabled ?" in block
    assert "reanalysis required" in block
    assert "select a source" in block


if __name__ == "__main__":
    main()
