#!/usr/bin/env python3
"""Portable contract for Library batch progress counts."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchProgressBar"')
    block = SOURCE[start : start + 1300]
    assert "batchCompletedCount" in block
    assert "batchFailedCount" in block
    assert "batchCancelledCount" in block
    assert "% complete;" in block


if __name__ == "__main__":
    main()
