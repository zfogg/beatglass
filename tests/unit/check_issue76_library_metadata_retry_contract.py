#!/usr/bin/env python3
"""Portable contract for the visible Library metadata retry action."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryMetadata"')
    block = SOURCE[start - 80 : SOURCE.index("                        }", start)]
    assert "id: retryMetadata" in block
    assert "window.metadataBridge.inspect(window.selectedAudioUrl)" in block
    assert "Metadata retry requested for" in block
    assert "retryMetadata.forceActiveFocus()" in block
    assert 'visible: window.selectedAudioUrl.toString() !== ""' in block
    assert 'window.metadataStatus.indexOf("Error:") === 0' in block


if __name__ == "__main__":
    main()
