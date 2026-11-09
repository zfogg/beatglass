#!/usr/bin/env python3
"""Portable regression contract for opening playback and stale evidence."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "openPlayback"')
    block = SOURCE[start : SOURCE.index('objectName: "playSelectedTrack"', start)]
    assert "const accepted = window.playbackBridge.openSource" in block
    assert "if (accepted) window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until fresh accepted analysis" in block


if __name__ == "__main__":
    main()
