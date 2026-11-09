#!/usr/bin/env python3
"""Portable contract for the visible Library clear-selection action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "clearSelectedAudio"')
    block = SOURCE[start - 80 : SOURCE.index("                        }", start)]
    assert "id: clearSelectedAudio" in block
    assert "window.clearSelectedAudio()" in block
    assert "Library selection cleared" in block
    assert "clearSelectedAudio.forceActiveFocus()" in block
    assert "enabled: window.selectedName !== \"\"" in block


if __name__ == "__main__":
    main()
