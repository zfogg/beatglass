#!/usr/bin/env python3
"""Portable contract for cache reanalysis confirmation focus recovery."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "reanalyzeInvalidatedCache"')
    block = SOURCE[start : SOURCE.index('objectName: "invalidateCacheDialog"', start)]
    assert "reanalyzeCacheDialog.open()" in block
    dialog = SOURCE[SOURCE.index('objectName: "reanalyzeCacheDialog"') : SOURCE.index('objectName: "invalidateCacheDialog"')]
    assert "standardButton(Dialog.Cancel).forceActiveFocus()" in dialog
    assert "reanalyzeInvalidatedCache.forceActiveFocus()" in dialog
    assert "source audio remains unchanged" in dialog


if __name__ == "__main__":
    main()
