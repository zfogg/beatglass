#!/usr/bin/env python3
"""Portable contract for empty-state Library folder chooser feedback."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "chooseFolderEmpty"')
    block = SOURCE[start : SOURCE.index("                        }", start)]
    assert "folderPicker.open()" in block
    assert "Choose a Library folder dialog opened from the empty state." in block
    assert "chooseFolderEmpty.forceActiveFocus()" in block
    assert "Accessible.description" in block


if __name__ == "__main__":
    main()
