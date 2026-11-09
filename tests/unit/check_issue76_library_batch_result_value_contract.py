#!/usr/bin/env python3
"""Portable contract for Library batch result accessible empty state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchLastResult"')
    block = SOURCE[start : start + 900]
    assert 'Accessible.value: visible ? text : "unavailable; no batch result"' in block
    assert "No batch analysis result is available" in block


if __name__ == "__main__":
    main()
