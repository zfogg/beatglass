#!/usr/bin/env python3
"""Portable contract for Library analysis queue recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisQueueProgress"')
    block = SOURCE[start : start + 1800]
    assert "Use Cancel, Retry, or discard recovery according to the reported state" in block
    assert "Analysis queue progress is unavailable" in block


if __name__ == "__main__":
    main()
