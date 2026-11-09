#!/usr/bin/env python3
"""Portable contract for Library analysis recovery discard focus."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "discardAnalysis"')
    block = SOURCE[start - 100 : SOURCE.index('objectName: "cancelAnalysis"', start)]
    assert "id: discardAnalysisButton" in block
    assert "discardAnalysisDialog.open()" in block
    assert "Confirm discard of analysis recovery" in block
    assert "onOpened: standardButton(Dialog.Cancel).forceActiveFocus()" in block
    assert "onRejected: discardAnalysisButton.forceActiveFocus()" in block
    assert "source audio will remain unchanged" in block


if __name__ == "__main__":
    main()
