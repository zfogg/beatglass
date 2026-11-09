#!/usr/bin/env python3
"""Portable contract for admitted analysis recovery focus."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "cancelAdmittedAnalysis"')
    block = SOURCE[start : SOURCE.index('objectName: "analysisCacheLookupControls"', start)]
    assert "cancelAdmittedAnalysis.forceActiveFocus()" in block
    assert "retryAdmittedAnalysis.forceActiveFocus()" in block
    assert "storageRecovery.cancelAnalysis()" in block
    assert "window.retryVisualizationAnalysis()" in block


if __name__ == "__main__":
    main()
