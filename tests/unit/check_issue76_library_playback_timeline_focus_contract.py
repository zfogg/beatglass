#!/usr/bin/env python3
"""Portable contract for Library playback timeline action focus."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def line(name: str) -> str:
    start = SOURCE.index('objectName: "' + name + '"')
    return SOURCE[start - 180 : SOURCE.index("\n", start)]


def main() -> None:
    pause = line("pausePlayback")
    seek = line("seekToFrame")
    assert "id: pausePlayback" in pause
    assert "window.togglePlaybackPause()" in pause
    assert "pausePlayback.forceActiveFocus()" in pause
    assert "id: seekToFrame" in seek
    assert "window.playbackBridge.seek(seekFrame.value" in seek
    assert "Playback seek requested to frame" in seek
    assert "seekToFrame.forceActiveFocus()" in seek


if __name__ == "__main__":
    main()
