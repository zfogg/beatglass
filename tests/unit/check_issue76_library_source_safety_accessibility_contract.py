#!/usr/bin/env python3
"""Portable contract for selected Library source safety messaging."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "selectedAudioSource"')
    block = SOURCE[start : SOURCE.index('objectName: "selectedAudioDuration"', start)]
    assert "read-only" in block
    assert "never writes tags or source audio" in block


if __name__ == "__main__":
    main()
