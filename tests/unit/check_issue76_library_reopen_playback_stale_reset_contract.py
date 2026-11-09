#!/usr/bin/env python3
"""Portable regression contract for reopening playback and stale evidence."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("function reopenSelectedPlayback()")
    block = SOURCE[start : SOURCE.index("    function retryNativePlaybackOutput", start)]
    assert "const reopened" in block
    assert "if (reopened)" in block
    assert "previous visualization evidence is stale until fresh accepted analysis" in block


if __name__ == "__main__":
    main()
