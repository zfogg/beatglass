#!/usr/bin/env python3
"""Portable contract for persisted recovery retry focus restoration."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryPersistedRecovery"')
    block = SOURCE[start : SOURCE.index('objectName: "discardAllPersistedRecovery"', start)]
    assert "storageRecovery.retrySelectedRun()" in block
    assert "retryPersistedRecovery.forceActiveFocus()" in block
    assert "forceActiveFocus()" in block


if __name__ == "__main__":
    main()
