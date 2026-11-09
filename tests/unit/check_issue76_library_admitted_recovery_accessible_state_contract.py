#!/usr/bin/env python3
"""Portable contract for admitted analysis recovery accessibility state."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "cancelAdmittedAnalysis"')
    block = SOURCE[start : SOURCE.index('objectName: "analysisCacheLookupControls"', start)]
    assert 'Accessible.value: enabled ? "available; " + storageRecovery.analysisStatus' in block
    assert 'Accessible.value: enabled ? "available; retryable"' in block
    assert "analysisStatus" in block


if __name__ == "__main__":
    main()
