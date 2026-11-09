#!/usr/bin/env python3
"""Portable contract for persisted Library recovery source safety."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "persistedRecoveryStatus"')
    block = SOURCE[start : start + 2200]
    assert "Source audio remains unchanged" in block
    assert "select one to retry or discard" in block


if __name__ == "__main__":
    main()
