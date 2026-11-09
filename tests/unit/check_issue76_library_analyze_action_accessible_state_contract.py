#!/usr/bin/env python3
"""Portable contract for Library analyze-action accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analyzeSelectedAudio"')
    block = SOURCE[start : SOURCE.index('objectName: "metadataStatus"', start)]
    assert '"available; " + window.selectedName' in block
    assert "unavailable; analysis active" in block
    assert "unavailable; select an audio file" in block


if __name__ == "__main__":
    main()
