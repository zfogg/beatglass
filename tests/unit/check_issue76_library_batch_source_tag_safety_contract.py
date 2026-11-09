#!/usr/bin/env python3
"""Portable contract for Library batch source tag safety guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchLastSource"')
    block = SOURCE[start : start + 900]
    assert "source remains read-only" in block
    assert "tags are never written" in block


if __name__ == "__main__":
    main()
