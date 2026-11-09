#!/usr/bin/env python3
"""Portable contract for Library batch source safety guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchLastSource"')
    block = SOURCE[start : start + 900]
    assert "most recently verified published batch result" in block
    assert "source remains read-only" in block
    assert "No verified batch source has been published" in block


if __name__ == "__main__":
    main()
