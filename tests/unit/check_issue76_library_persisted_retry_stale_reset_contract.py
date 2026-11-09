#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on persisted retry."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("storageRecovery.retrySelectedRun()", SOURCE.index('objectName: "retryPersistedRecovery"'))
    block = SOURCE[start : start + 800]
    assert "window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until completion" in block


if __name__ == "__main__":
    main()
