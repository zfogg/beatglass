#!/usr/bin/env python3
"""Portable contract for bulk persisted recovery discard accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "discardAllPersistedRecovery"')
    block = SOURCE[start : SOURCE.index('objectName: "discardAllRecoveryDialog"', start)]
    assert "Accessible.value: enabled ?" in block
    assert "retryableRunIds.length" in block
    assert "unavailable; no retryable runs" in block


if __name__ == "__main__":
    main()
