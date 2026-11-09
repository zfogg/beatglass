#!/usr/bin/env python3
"""Portable contract for selected-duration metadata recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "selectedAudioDuration"')
    block = SOURCE[start : SOURCE.index('objectName: "clearSelectedAudio"', start)]
    assert 'metadataStatus.indexOf("Error:")' in block
    assert "Retry metadata" in block


if __name__ == "__main__":
    main()
