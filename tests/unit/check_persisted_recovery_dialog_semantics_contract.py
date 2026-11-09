"""Contract checks for persisted recovery discard dialog semantics."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "discardRecoveryDialog"',
        'Accessible.name: "Discard saved analysis run confirmation"',
        'Accessible.description: "Confirm to remove only the selected retry record. The source audio file is never changed. Cancel returns focus to the discard control."',
        'Accessible.name: "Discard retryable run confirmation"',
        'Accessible.description: "The selected retry record will be removed; source audio remains unchanged."',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery discarded; selected retry state cleared.")',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept."); discardPersistedRecoveryButton.forceActiveFocus() }',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery dialog semantics markers: " + ", ".join(missing))
    print(f"persisted recovery dialog semantics contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
