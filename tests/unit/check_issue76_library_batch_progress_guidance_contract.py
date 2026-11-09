#!/usr/bin/env python3
"""Portable contract for Library batch progress recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchProgressBar"')
    block = SOURCE[start : start + 1800]
    assert "Cancel is available while running" in block
    assert "retry or discard recovery" in block
    assert "Batch progress is unavailable" in block


if __name__ == "__main__":
    main()
