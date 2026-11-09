#!/usr/bin/env python3
"""Portable contract for Library empty batch progress state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchProgressBar"')
    block = SOURCE[start : start + 1500]
    assert "unavailable; no accepted batch items" in block
    assert "in progress;" in block
    assert "batchCompletedCount" in block


if __name__ == "__main__":
    main()
