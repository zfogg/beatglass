#!/usr/bin/env python3
"""Portable contract for Library cache lookup feedback and focus recovery."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "lookupAnalysisCache"')
    block = SOURCE[start : SOURCE.index('objectName: "invalidateAnalysisCache"', start)]
    assert "storageRecovery.findCached" in block
    assert "Cache lookup requested" in block
    assert "lookupAnalysisCache.forceActiveFocus()" in block


if __name__ == "__main__":
    main()
