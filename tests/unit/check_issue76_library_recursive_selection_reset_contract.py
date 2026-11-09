#!/usr/bin/env python3
"""Portable contract for clearing selection when leaving recursive results."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert SOURCE.count("window.clearSelectedAudio()") >= 3
    assert "filter, entry selection, and selected source were cleared" in SOURCE
    assert 'objectName: "libraryRecursiveReturnShortcut"' in SOURCE
    assert 'objectName: "returnToFolderBrowser"' in SOURCE


if __name__ == "__main__":
    main()
