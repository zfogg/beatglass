#!/usr/bin/env python3
"""Portable contract for Library terminal recovery next-action guidance."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analysisRecoveryNextAction"')
    block = SOURCE[start : start + 1000]
    assert "batchAcceptedCount" in block
    assert "retried" in block
    assert "without changing source audio" in block


if __name__ == "__main__":
    main()
