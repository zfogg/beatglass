#!/usr/bin/env python3
"""Portable contract for Library batch action count guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisBatchActionStatus"')
    block = SOURCE[start : start + 1400]
    assert "batchAcceptedCount" in block
    assert "accepted item(s) are represented" in block
    assert "retain source audio" in block


if __name__ == "__main__":
    main()
