#!/usr/bin/env python3
"""Portable contract for Library indeterminate batch progress state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchProgressBar"')
    block = SOURCE[start : start + 1400]
    assert 'indeterminate ? "in progress; "' in block
    assert "batchCompletedCount" in block
    assert "batchCancelledCount" in block


if __name__ == "__main__":
    main()
