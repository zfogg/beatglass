#!/usr/bin/env python3
"""Portable contract for bulk persisted-recovery discard confirmation."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "discardAllPersistedRecovery"')
    block = SOURCE[start : SOURCE.index('objectName: "discardAllRecoveryDialog"', start)]
    assert "discardAllRecoveryDialog.open()" in block
    assert "source audio remains unchanged" in block
    assert "discardAllPersistedRecovery" in SOURCE[SOURCE.index('objectName: "discardAllRecoveryDialog"') :]


if __name__ == "__main__":
    main()
