#!/usr/bin/env python3
"""Portable contract for actionable Library metadata error guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "metadataErrorGuidance"')
    block = SOURCE[start : SOURCE.index('objectName: "detailsAvailability"', start)]
    assert "Retry metadata" in block
    assert "Clear selection" in block
    assert "Accessible.description: text +" in block


if __name__ == "__main__":
    main()
