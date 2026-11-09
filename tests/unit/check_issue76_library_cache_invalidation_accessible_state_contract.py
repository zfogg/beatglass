#!/usr/bin/env python3
"""Portable contract for Library cache invalidation accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "invalidateAnalysisCache"')
    block = SOURCE[start : SOURCE.index('objectName: "reanalyzeInvalidatedCache"', start)]
    assert "Accessible.value: enabled ?" in block
    assert "completed cache hit" in block
    assert "run Check cache" in block


if __name__ == "__main__":
    main()
