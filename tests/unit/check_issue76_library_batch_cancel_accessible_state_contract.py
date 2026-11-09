#!/usr/bin/env python3
"""Portable contract for Library batch cancel accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "cancelAnalysisBatch"')
    block = SOURCE[start : start + 1500]
    assert 'enabled ? "available; "' in block
    assert "unavailable;" in block
    assert "accepted items" in block
    assert "Batch cancel is unavailable" in block


if __name__ == "__main__":
    main()
