#!/usr/bin/env python3
"""Portable contract for persisted recovery discard accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("id: discardPersistedRecoveryButton")
    block = SOURCE[start : SOURCE.index('objectName: "retryPersistedRecovery"', start)]
    assert "Accessible.value: enabled ?" in block
    assert "storageRecovery.selectedRunId" in block
    assert "analysisStatus" in block


if __name__ == "__main__":
    main()
