#!/usr/bin/env python3
"""Portable regression contract for playing a selected Library source."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("function playSelectedLibraryTrack()")
    block = SOURCE[start : SOURCE.index("    function activateEntry", start)]
    assert block.count("const accepted = window.playbackBridge.openSource") == 2
    assert block.count("clearVisualizationForCaptureState") == 2
    assert "fresh accepted analysis" in block


if __name__ == "__main__":
    main()
