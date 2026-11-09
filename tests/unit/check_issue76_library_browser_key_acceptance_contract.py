#!/usr/bin/env python3
"""Portable contract for Library browser key event consumption."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('id: browser')
    block = SOURCE[start : SOURCE.index("Keys.onBackspacePressed", start)]
    for key in ("Keys.onReturnPressed", "Keys.onEnterPressed", "Keys.onSpacePressed"):
        assert key in block
    assert block.count("event.accepted = true") >= 3
    assert block.count("window.activateEntry(currentIndex)") >= 3


if __name__ == "__main__":
    main()
