#!/usr/bin/env python3
"""Portable contract for cache invalidation confirmation focus recovery."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "invalidateAnalysisCache"')
    end = SOURCE.index('objectName: "reanalyzeInvalidatedCache"', start)
    action = SOURCE[start:end]
    assert "invalidateCacheDialog.open()" in action
    dialog_start = SOURCE.index('objectName: "invalidateCacheDialog"')
    dialog = SOURCE[dialog_start : SOURCE.index('objectName: "persistedRecoveryRuns"', dialog_start)]
    assert "standardButton(Dialog.Cancel).forceActiveFocus()" in dialog
    assert "invalidateAnalysisCache.forceActiveFocus()" in dialog
    assert "source audio remains unchanged" in dialog


if __name__ == "__main__":
    main()
