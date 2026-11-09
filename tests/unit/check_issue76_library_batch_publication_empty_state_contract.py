#!/usr/bin/env python3
"""Portable contract for Library batch publication empty state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchPublicationSummary"')
    block = SOURCE[start : start + 1500]
    assert "no accepted items are available for publication" in block
    assert "terminal failures" in block


if __name__ == "__main__":
    main()
