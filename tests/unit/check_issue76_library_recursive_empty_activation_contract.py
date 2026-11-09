#!/usr/bin/env python3
"""Portable contract for empty recursive-result activation feedback."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("id: recursiveResultsView")
    block = SOURCE[start : SOURCE.index('objectName: "selectedAudioPanel"', start)]
    assert block.count("Choose a recursive Library result before activating.") == 3
    assert block.count("currentIndex >= 0") >= 3
    assert block.count("event.accepted = true") >= 4


if __name__ == "__main__":
    main()
