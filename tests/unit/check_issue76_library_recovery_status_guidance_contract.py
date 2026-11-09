#!/usr/bin/env python3
"""Portable contract for Library analysis recovery status guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisRecoveryStatus"')
    block = SOURCE[start : start + 800]
    assert "Review admission and queue status" in block
    assert "Retry or discard recovery" in block
    assert "No recovery reset has been performed" in block


if __name__ == "__main__":
    main()
