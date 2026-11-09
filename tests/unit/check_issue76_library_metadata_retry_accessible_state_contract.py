#!/usr/bin/env python3
"""Portable contract for metadata retry accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryMetadata"')
    block = SOURCE[start : SOURCE.index('objectName: "metadataErrorGuidance"', start)]
    assert 'Accessible.value: enabled ? "available; "' in block
    assert "retry cooldown active" in block
    assert "metadataRetryPending" in block


if __name__ == "__main__":
    main()
