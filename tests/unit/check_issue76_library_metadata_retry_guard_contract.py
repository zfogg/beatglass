#!/usr/bin/env python3
"""Portable contract for guarding repeated metadata recovery requests."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryMetadata"')
    block = SOURCE[start : SOURCE.index('objectName: "metadataErrorGuidance"', start)]
    assert "!window.metadataRetryPending" in block
    assert "window.metadataRetryPending = true" in block
    assert "metadataRetryCooldown.restart()" in block
    timer = SOURCE[SOURCE.index("id: metadataRetryCooldown") :]
    assert "interval: 750" in timer
    assert "window.metadataRetryPending = false" in timer


if __name__ == "__main__":
    main()
