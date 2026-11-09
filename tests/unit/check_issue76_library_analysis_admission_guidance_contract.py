#!/usr/bin/env python3
"""Portable contract for Library analysis admission recovery guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisAdmissionStatus"')
    block = SOURCE[start : start + 800]
    assert "Queued or running work can be cancelled" in block
    assert "terminal failures can be retried or discarded" in block
    assert "Analysis admission status is unavailable" in block


if __name__ == "__main__":
    main()
