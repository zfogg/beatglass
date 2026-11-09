#!/usr/bin/env python3
"""Portable contract for Library playback lifecycle controls."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def line(name: str) -> str:
    start = SOURCE.index('objectName: "' + name + '"')
    return SOURCE[start - 180 : SOURCE.index("\n", start)]


def main() -> None:
    open_line = line("openPlayback")
    start_line = line("startNativePlayback")
    stop_line = line("stopPlayback")
    assert "openPlayback.forceActiveFocus()" in open_line
    assert "announceCaptureRecoveryState" in open_line
    assert "startNativePlayback.forceActiveFocus()" in start_line
    assert "window.startNativePlaybackOutput()" in start_line
    assert "stopPlayback.forceActiveFocus()" in stop_line
    assert "window.stopPlaybackWithRecovery" in stop_line


if __name__ == "__main__":
    main()
