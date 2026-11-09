#!/usr/bin/env python3
"""Portable regression contract for the Library Reopen selected workflow."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("function reopenSelectedLibraryTrack()")
    block = SOURCE[start : SOURCE.index("    function activateEntry", start)]
    assert "window.playbackBridge.openSource" in block
    assert "if (accepted)" in block
    assert "previous visualization evidence is stale until fresh accepted analysis" in block


if __name__ == "__main__":
    main()
