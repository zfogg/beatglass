from pathlib import Path


QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

MARKERS = {
    "resume shortcut object": 'objectName: "visualizationResumeShortcut"',
    "resume shortcut sequence": 'sequence: "Ctrl+Alt+R"',
    "resume shortcut enabled": 'enabled: window.playbackBridge !== null && window.playbackBridge.state === "Paused"',
    "resume shortcut action": "window.playbackBridge.resume(); window.announceVisualizationRecovery",
    "stop shortcut object": 'objectName: "visualizationStopShortcut"',
    "stop shortcut sequence": 'sequence: "Ctrl+Alt+Shift+S"',
    "stop shortcut enabled": 'enabled: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped"',
    "stop shortcut action": "window.playbackBridge.stop(); window.announceVisualizationRecovery",
    "resume button object": 'objectName: "resumeVisualizationPlayback"',
    "resume button action": "window.playbackBridge.resume(); window.announceVisualizationRecovery",
    "stop button object": 'objectName: "stopVisualizationPlayback"',
    "stop button action": "window.playbackBridge.stop(); window.announceVisualizationRecovery",
}


def main() -> None:
    missing = [name for name, marker in MARKERS.items() if marker not in QML]
    if missing:
        raise SystemExit("Missing playback shortcut markers: " + ", ".join(missing))
    print(f"Playback shortcut contract markers verified ({len(MARKERS)} checks)")


if __name__ == "__main__":
    main()
