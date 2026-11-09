#!/usr/bin/env python3
"""Portable regression contract for clearing stale evidence on worker discard."""

from pathlib import Path

SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index("window.analysisJobBridge.discard();")
    block = SOURCE[start : start + 500]
    assert "window.clearVisualizationForCaptureState" in block
    assert "previous visualization evidence is stale until a new accepted result" in block


if __name__ == "__main__":
    main()
