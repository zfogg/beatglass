"""Contract checks for selected identity in recovery discard announcements."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Discard saved retryable analysis run " + storageRecovery.selectedRunId',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        '"Discard retryable run " + storageRecovery.selectedRunId + " confirmation"',
        'onOpened: { window.announceVisualizationRecovery("Confirm discard for persisted recovery run " + storageRecovery.selectedRunId',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery identity announcement markers: " + ", ".join(missing))
    print(f"persisted recovery identity announcement contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
