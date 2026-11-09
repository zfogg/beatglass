#!/usr/bin/env python3
"""Portable contract for canceled Library folder selection."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("id: folderPicker")
    block = SOURCE[start:]
    assert "onRejected:" in block
    assert "Library folder selection canceled; the current folder remains open." in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "onAccepted:" in block
    assert "Library folder selected; choose an audio file to inspect or analyze." in block


if __name__ == "__main__":
    main()
