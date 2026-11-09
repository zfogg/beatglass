#!/usr/bin/env python3
"""Portable contract for Library native playback recovery focus."""

from pathlib import Path


SOURCE = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = SOURCE.index('objectName: "retryNativePlayback"')
    line = SOURCE[start - 100 : SOURCE.index("\n", start)]
    assert "id: retryNativePlayback" in line
    assert "window.retryNativePlaybackOutput()" in line
    assert "retryNativePlayback.forceActiveFocus()" in line
    assert "nativeError !== \"\"" in line
    function = SOURCE[SOURCE.index("function retryNativePlaybackOutput") :]
    assert "announceCaptureRecoveryState" in function


if __name__ == "__main__":
    main()
