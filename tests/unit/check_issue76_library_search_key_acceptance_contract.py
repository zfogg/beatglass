#!/usr/bin/env python3
"""Portable contract for Library search key event consumption."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('id: search')
    block = SOURCE[start : SOURCE.index('objectName: "sortOrder"', start)]
    assert "onAccepted:" in block
    assert "files.count + \" Library entries match the filter.\"" in block
    assert "event.accepted = true" in block
    assert "Keys.onEscapePressed:" in block
    assert "Library filter cleared; all folder entries are visible." in block


if __name__ == "__main__":
    main()
