from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/desktop/playback_session_bridge.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "src/desktop/playback_session_bridge.cpp").read_text(encoding="utf-8")
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


MARKERS = {
    "source notify property": "Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged)",
    "epoch notify property": "Q_PROPERTY(qulonglong epoch READ epoch NOTIFY epochChanged)",
    "source signal": "void sourceChanged();",
    "epoch signal": "void epochChanged();",
    "state signal": "void stateChanged();",
    "source transition emission": "emit sourceChanged(); emit epochChanged(); emit stateChanged();",
    "stop transition emission": "source_ = QUrl{}; emit sourceChanged(); emit epochChanged(); emit stateChanged();",
    "pause transition emission": "if (ok) { emit stateChanged(); emit changed(); }",
    "seek epoch emission": "if (ok) { emit epochChanged(); emit changed(); }",
    "qml source reset": "function onSourceChanged() { window.clearVisualizationForCaptureState",
    "qml epoch reset": "function onEpochChanged() { window.clearVisualizationForCaptureState",
    "qml state reset": "function onStateChanged() {",
    "qml stop invalidation reason": 'Playback stopped; accepted visualization snapshot cleared.',
    "qml pause resume bridge sync": "window.syncVisualizationSelectionFromPlayback(window.playbackBridge.currentFrame",
    "qml capture retry state": "Capture retry failed · check permission and selected device",
    "qml no device state": "No output device detected · connect a device and refresh endpoints",
    "qml endpoint stale clear": "Selection cleared because the capture endpoint changed.",
    "qml endpoint retry failure": "Capture endpoint retry failed; check permission and reacquire a device.",
    "qml overlay recovery": "DJ overlay unavailable; output monitoring remains unavailable until the display recovers.",
    "qml overlay hide recovery": "DJ overlay hidden; output monitoring remains available in the control window.",
    "qml overlay display recovery": "DJ overlay display selected; retry is now available.",
    "qml overlay display disappeared": "DJ overlay display disappeared; controls restored until a display is available.",
    "qml overlay display restored": "DJ overlay display restored; choose a display to resume overlay monitoring.",
    "qml overlay retry announcement": "DJ overlay retry requested; control window remains visible until the overlay is restored.",
    "qml overlay selection gate": "overlayDisplaySelectionRequired",
    "qml overlay selection readiness": "DJ overlay display selected; retry is now available.",
    "qml overlay persisted diagnostic": "Persisted analysis admission result (not current visualization):",
    "qml overlay diagnostic dismissal": "!window.overlayPersistedDiagnosticDismissed",
}


def main() -> None:
    text = HEADER + SOURCE + QML
    missing = [name for name, marker in MARKERS.items() if marker not in text]
    if missing:
        raise SystemExit("Missing playback transition bridge markers: " + ", ".join(missing))
    print(f"Playback transition bridge markers verified ({len(MARKERS)} checks)")


if __name__ == "__main__":
    main()
