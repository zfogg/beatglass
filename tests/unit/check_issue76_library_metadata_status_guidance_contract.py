#!/usr/bin/env python3
"""Portable contract for Library metadata status recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "metadataStatus"')
    block = SOURCE[start : start + 800]
    assert 'metadataStatus.indexOf("Error:")' in block
    assert "Retry metadata is available" in block


if __name__ == "__main__":
    main()
