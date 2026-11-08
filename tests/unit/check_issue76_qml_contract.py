"""Static regression checks for the issue #76 QML acceptance contract.

This complements qmltestrunner on hosts where Qt is unavailable. It checks
that the production QML still exposes the interaction and recovery hooks that
the visual audit documents; it does not replace native or runtime testing.
"""

from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    qml = (root / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
    required = {
        "compact responsive header": "compactHeader",
        "visualization focus ring": "visualizationFocusRing",
        "visualization keyboard selection": "Keys.onRightPressed",
        "visualization mode arrow focus": "spectrumView.forceActiveFocus()",
        "visualization shortcut focus": "waveformView.forceActiveFocus()",
        "visualization keyboard guidance": "Use Left and Right arrows to switch views",
        "capture control accessible name": "Accessible.name: text; Accessible.description: window.captureMonitorBridge",
        "playback pause accessible name": "Accessible.name: text + \" playback\"",
        "playback stop accessible name": "Accessible.name: \"Stop playback\"",
        "playback seek accessible names": "Accessible.name: \"Seek backward five seconds\"",
        "native playback accessible names": "Accessible.name: \"Start native playback\"",
        "playback device current selection": "Current output device: \" + currentText",
        "native playback selected format": "Start native playback at \" + playbackSampleRate.value",
        "overlay action accessible names": "Accessible.name: \"Show DJ overlay\"",
        "overlay retry accessible name": "Accessible.name: \"Retry DJ overlay\"",
        "overlay unlock accessible name": "Accessible.name: \"Unlock DJ overlay\"",
        "visualization empty state": "visualizationEmptyState",
        "generation/revision status": "visualizationRevisionStatus",
        "playback discontinuity clear": "onEpochChanged()",
        "overlay empty display state": "overlayDisplayEmptyState",
    }
    missing = [name for name, marker in required.items() if marker not in qml]
    if missing:
        raise SystemExit("Missing issue #76 QML contract markers: " + ", ".join(missing))
    print(f"Issue #76 QML contract markers verified ({len(required)} checks)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
