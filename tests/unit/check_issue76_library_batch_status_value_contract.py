#!/usr/bin/env python3
"""Portable contract for Library batch status accessible empty state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchStatus"')
    block = SOURCE[start : start + 2400]
    assert 'Accessible.value: visible ? text : "unavailable; analysis bridge not reporting"' in block
    assert "Batch queue status is unavailable" in block


if __name__ == "__main__":
    main()
