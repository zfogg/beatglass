#!/usr/bin/env python3
"""Portable contract for Library batch publication accessible value."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchPublicationSummary"')
    block = SOURCE[start : start + 1600]
    assert 'Accessible.value: visible ? text : "unavailable; no accepted items"' in block
    assert "No analysis batch is active" in block


if __name__ == "__main__":
    main()
