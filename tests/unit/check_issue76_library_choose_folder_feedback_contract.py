#!/usr/bin/env python3
"""Portable contract for Library folder chooser feedback."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "chooseFolder"')
    block = SOURCE[start - 60 : SOURCE.index(" }", start)]
    assert "folderPicker.open()" in block
    assert "Choose a Library folder dialog opened." in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "activeFocusOnTab: true" in block


if __name__ == "__main__":
    main()
