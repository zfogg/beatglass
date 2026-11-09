#!/usr/bin/env python3
"""Portable regression contract for clearing stale Library analysis on queue."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "analyzeSelectedAudio"')
    block = SOURCE[start : SOURCE.index('objectName: "metadataStatus"', start)]
    assert "const accepted = window.analysisJobBridge.submitSelectedFile" in block
    assert "if (accepted)" in block
    assert "previous visualization evidence is stale until completion" in block


if __name__ == "__main__":
    main()
