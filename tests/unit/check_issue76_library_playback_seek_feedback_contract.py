#!/usr/bin/env python3
"""Portable contract for Library playback seek feedback."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def button(name: str) -> str:
    start = SOURCE.index('objectName: "' + name + '"')
    return SOURCE[start - 80 : SOURCE.index("\n", start)]


def main() -> None:
    back = button("seekBackPlayback")
    forward = button("seekForwardPlayback")
    assert "id: seekBackPlayback" in back
    assert "Playback sought backward" in back
    assert "seekBackPlayback.forceActiveFocus()" in back
    assert "id: seekForwardPlayback" in forward
    assert "Playback sought forward" in forward
    assert "seekForwardPlayback.forceActiveFocus()" in forward


if __name__ == "__main__":
    main()
