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
    "qml overlay display recovery": "DJ overlay display selection persisted for ",
    "qml overlay display disappeared": "DJ overlay display disappeared; controls restored until a display is available.",
    "qml overlay display restored": "DJ overlay display restored; choose a display to resume overlay monitoring.",
    "qml overlay retry announcement": "DJ overlay retry requested; control window remains visible until the overlay is restored.",
    "qml overlay selection gate": "overlayDisplaySelectionRequired",
    "qml overlay selection readiness": "persistOverlayDisplaySelection(currentIndex, currentText)",
    "qml overlay persisted diagnostic": "Persisted analysis admission result (not current visualization):",
    "qml overlay diagnostic dismissal": "!window.overlayPersistedDiagnosticDismissed",
    "qml overlay saved target": "overlayPreferredScreenIndex",
    "qml overlay saved display name": "overlayPreferredDisplayName",
    "qml overlay display name restore": "saved display target selected by name.",
    "qml overlay missing display recovery": 'objectName: "overlayMissingDisplayRecovery"',
    "qml overlay forget display": 'objectName: "forgetUnavailableOverlayDisplay"',
    "qml overlay forget announcement": "Saved overlay display cleared; choose an available display to restore monitoring.",
    "qml overlay persist display": "function persistOverlayDisplaySelection(index, name)",
    "qml overlay persist announcement": "DJ overlay display selection persisted for ",
    "qml overlay relaunch hydration": "function restoreOverlayDisplaySelectionFromBridge()",
    "qml overlay stale selection": "savedIndex < 0",
    "qml overlay visual hydration": "function restoreOverlayVisualStateFromBridge()",
    "qml overlay visual fallback": "Persisted overlay geometry unavailable; control-window fallback remains active.",
    "qml overlay invalid opacity": "Persisted overlay opacity unavailable; safe 90 percent opacity applied.",
    "qml overlay show hydration": "window.restoreOverlayVisualStateFromBridge()",
    "qml overlay target restoration": "saved display target selected by name.",
    "qml overlay click-through state": "DJ overlay is click-through; unlock it to restore keyboard and pointer interaction.",
    "qml overlay opacity state": "DJ overlay opacity set to ",
    "qml overlay size state": "DJ overlay size set to ",
    "qml overlay fallback": "Control window remains available as the tray/taskbar fallback until the overlay display recovers.",
    "qml overlay geometry status": 'objectName: "overlayGeometryStatus"',
    "qml overlay geometry announcement": "DJ overlay geometry restored at ",
    "qml overlay restore state": 'objectName: "overlayRestoreState"',
    "qml overlay restore fallback": "Persisted restore state unavailable; control-window fallback remains active.",
    "qml overlay reapply control": 'objectName: "reapplyOverlaySettings"',
    "qml overlay reapply announcement": "Persisted overlay display, geometry, and opacity reapplied.",
    "qml overlay unlock helper": "function unlockOverlayAndRestoreFocus()",
    "qml overlay unlock announcement": "DJ overlay unlocked; keyboard and pointer interaction restored.",
    "qml overlay relaunch geometry": "DJ overlay relaunched at its persisted position and size.",
    "qml overlay relaunch fallback": "DJ overlay could not be restored; control window remains available as the tray/taskbar fallback.",
    "qml overlay restore affordance": 'objectName: "restoreOverlay"',
    "qml overlay quit control": 'objectName: "quitBeatglass"',
    "qml overlay quit lifecycle": "Beatglass is quitting; overlay hidden and persisted settings retained.",
}


def main() -> None:
    text = HEADER + SOURCE + QML
    missing = [name for name, marker in MARKERS.items() if marker not in text]
    if missing:
        raise SystemExit("Missing playback transition bridge markers: " + ", ".join(missing))
    print(f"Playback transition bridge markers verified ({len(MARKERS)} checks)")


if __name__ == "__main__":
    main()
