#!/usr/bin/env python3
"""Portable contract for recursive Library selection synchronization."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('id: recursiveResultsView')
    block = SOURCE[start : SOURCE.index('objectName: "selectedAudioPanel"', start)]
    assert "recursiveResultsView.currentIndex = index" in block
    assert "window.selectedName = name" in block
    assert "window.selectedAudioUrl = Qt.resolvedUrl(path)" in block
    assert "metadataBridge.inspect(window.selectedAudioUrl)" in block
    assert "analyzeSelectedAudio.forceActiveFocus()" in block
    assert "Selected recursive audio source" in block


if __name__ == "__main__":
    main()
