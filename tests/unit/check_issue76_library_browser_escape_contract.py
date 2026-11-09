#!/usr/bin/env python3
"""Portable contract for Escape recovery in the Library browser."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    marker = 'Keys.onEscapePressed: { window.clearSelectedAudio(); search.text = ""; browser.currentIndex = -1; search.forceActiveFocus();'
    assert marker in SOURCE
    assert "Library selection and filter cleared; all folder entries are visible." in SOURCE


if __name__ == "__main__":
    main()
