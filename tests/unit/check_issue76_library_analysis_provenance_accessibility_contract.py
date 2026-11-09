#!/usr/bin/env python3
"""Portable contract for Library analysis provenance guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisProvenance"')
    block = SOURCE[start : start + 900]
    assert "evidence provenance" in block
    assert "unavailable until analysis is queued" in block


if __name__ == "__main__":
    main()
