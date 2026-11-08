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
        "playback status static role": "objectName: \"playbackState\"; Accessible.name: \"Playback state: \" + text; Accessible.role: Accessible.StaticText",
        "overlay retry accessible name": "Accessible.name: \"Retry DJ overlay\"",
        "overlay unlock accessible name": "Accessible.name: \"Unlock DJ overlay\"",
        "click-through action accessible name": "Accessible.name: checked ? \"Disable click-through\"",
        "refresh outputs accessible name": "Accessible.name: \"Refresh playback outputs\"",
        "open playback accessible name": "Accessible.name: \"Open selected playback source\"",
        "playback frame actions accessible names": "Accessible.name: \"Seek to entered frame\"",
        "folder picker accessible name": "Accessible.name: \"Choose audio folder\"",
        "capture endpoint refresh accessible name": "Accessible.name: \"Refresh capture endpoints\"",
        "capture endpoint current selection": "Current capture endpoint: \" + currentText",
        "parent folder accessible name": "Accessible.name: \"Go to parent folder\"",
        "browser keyboard guidance": "Use Up and Down to select an entry",
        "library empty state role": "Accessible.role: Accessible.StaticText",
        "visualization status role": "Accessible.name: \"Visualization data status\"",
        "selected visualization status role": "Accessible.name: \"Selected visualization data status: \" + text",
        "capture status role": "Accessible.name: \"Capture monitor status: \" + text",
        "live monitoring status role": "Accessible.name: \"Live output monitoring\"",
        "metadata status role": "Accessible.name: \"Audio metadata status: \" + text",
        "current folder role": "Accessible.name: \"Current folder: \" + text",
        "visualization empty state": "visualizationEmptyState",
        "generation/revision status": "visualizationRevisionStatus",
        "playback discontinuity clear": "onEpochChanged()",
        "overlay empty display state": "overlayDisplayEmptyState",
        "overlay status static roles": "objectName: \"overlayInteractionMode\"; text: window.overlayBridge",
        "library capture status roles": "objectName: \"endpointStatus\"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: \"Output endpoint status: \" + text; Accessible.role: Accessible.StaticText",
        "library detail status roles": "objectName: \"selectedAudioName\"",
        "analysis status roles": "objectName: \"analysisProgress\"",
        "visual status roles": "objectName: \"visualizationLoadState\"",
        "recovery status roles": "objectName: \"playbackSeekLimitation\"",
        "dj status roles": "objectName: \"djStatus\"; text: \"Status: \" + window.liveBridge.status",
        "settings status roles": "objectName: \"settingsStatus\"",
        "generation status role": "objectName: \"visualizationRevisionStatus\"",
        "canvas selection announcement": "Selected position \" + Math.round(visualizationCanvas.selectedPosition * 100)",
        "dynamic sort and output descriptions": "Current order: \" + (window.sortDescending ? \"descending\"",
        "dynamic visualization and playback names": "Accessible.name: \"Live \" + (visualizationCanvas.viewMode",
        "dynamic overlay status names": "Accessible.name: \"Overlay interaction mode: \" + text",
        "dynamic capture and settings names": "Accessible.name: \"Capture monitor status: \" + text",
        "dynamic visualization status names": "Accessible.name: \"Visualization load state: \" + text",
        "dynamic analysis status names": "Accessible.name: \"Analysis progress: \" + text",
        "dynamic metadata names": "Accessible.name: \"Selected audio file: \" + text",
        "final dynamic status names": "Accessible.name: \"Visualization mode: \" + text",
        "screen keyboard shortcuts": "objectName: \"libraryTabShortcut\"",
        "screen shortcut focus management": "libraryTab.forceActiveFocus()",
        "screen tab focus rings": "libraryTab.activeFocus ? 3 : 1",
        "primary control tab focus": "objectName: \"toggleMonitoring\"; activeFocusOnTab: true",
        "secondary control tab focus": "objectName: \"playbackDevice\"; model: window.playbackBridge ? window.playbackBridge.devices : []; activeFocusOnTab: true",
        "combo role consistency": "Accessible.role: Accessible.ComboBox",
    }
    missing = [name for name, marker in required.items() if marker not in qml]
    if missing:
        raise SystemExit("Missing issue #76 QML contract markers: " + ", ".join(missing))
    print(f"Issue #76 QML contract markers verified ({len(required)} checks)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
