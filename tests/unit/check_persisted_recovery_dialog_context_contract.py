"""Contract checks for selected-run context in persisted discard confirmation."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'title: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Discard saved analysis run " + storageRecovery.selectedRunId + "?"',
        'onOpened: window.announceVisualizationRecovery("Confirm discard for persisted recovery run " + storageRecovery.selectedRunId + ". Source audio will remain unchanged.")',
        'Accessible.name: "Discard saved analysis run confirmation"',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery dialog context markers: " + ", ".join(missing))
    print(f"persisted recovery dialog context contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
