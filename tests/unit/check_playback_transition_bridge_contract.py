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
}


def main() -> None:
    text = HEADER + SOURCE + QML
    missing = [name for name, marker in MARKERS.items() if marker not in text]
    if missing:
        raise SystemExit("Missing playback transition bridge markers: " + ", ".join(missing))
    print(f"Playback transition bridge markers verified ({len(MARKERS)} checks)")


if __name__ == "__main__":
    main()
