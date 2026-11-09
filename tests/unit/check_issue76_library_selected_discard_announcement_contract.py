#!/usr/bin/env python3
"""Portable contract for selected persisted-recovery discard confirmation."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("id: discardPersistedRecoveryButton")
    block = SOURCE[start : SOURCE.index('objectName: "retryPersistedRecovery"', start)]
    assert "discardRecoveryDialog.open()" in block
    assert "selected saved recovery run" in block
    assert "source audio remains unchanged" in block


if __name__ == "__main__":
    main()
