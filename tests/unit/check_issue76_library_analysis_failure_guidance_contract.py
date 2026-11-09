#!/usr/bin/env python3
"""Portable contract for Library analysis failure recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisFailureReason"')
    block = SOURCE[start : start + 700]
    assert "Retry analysis" in block
    assert "discard recovery" in block
    assert "No analysis failure has been reported" in block


if __name__ == "__main__":
    main()
