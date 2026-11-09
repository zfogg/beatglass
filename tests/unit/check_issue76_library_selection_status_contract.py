#!/usr/bin/env python3
"""Portable contract for the Library selection status surface."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "librarySelectionStatus"')
    block = SOURCE[start : SOURCE.index("                }", start)]
    assert 'window.selectedAudioUrl.toString() === ""' in block
    assert "window.selectedName" in block
    assert 'Accessible.role: Accessible.Status' in block
    assert "Playback and analysis actions apply to this source." in block
    assert 'objectName: "libraryClearShortcut"' in SOURCE
    assert 'browser.forceActiveFocus()' in SOURCE[SOURCE.index('objectName: "libraryClearShortcut"') :]


if __name__ == "__main__":
    main()
