#!/usr/bin/env python3
"""Portable contract for Library double-click activation consumption."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    browser = SOURCE[SOURCE.index('objectName: "browser"') : SOURCE.index('objectName: "browserEmptyState"')]
    recursive = SOURCE[SOURCE.index('id: recursiveResultsView') : SOURCE.index('objectName: "selectedAudioPanel"')]
    assert "onDoubleClicked:" in browser
    assert "playSelectedTrack.click()" in browser
    assert "event.accepted = true" in browser
    assert "onDoubleClicked:" in recursive
    assert "event.accepted = true" in recursive


if __name__ == "__main__":
    main()
