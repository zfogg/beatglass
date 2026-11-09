#!/usr/bin/env python3
"""Portable contract for Library batch discard accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "discardAnalysisBatch"')
    block = SOURCE[start : start + 1100]
    assert 'enabled ? "available; "' in block
    assert "unavailable;" in block
    assert "accepted items" in block
    assert "without changing source audio" in block


if __name__ == "__main__":
    main()
