"""Contract checks for keyboard semantics on persisted recovery confirmation."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'standardButtons: Dialog.Ok | Dialog.Cancel',
        'Accessible.description: "Confirm to remove only the selected retry record. Enter confirms; Escape cancels. The source audio file is never changed. Cancel returns focus to the discard control."',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
        'discardPersistedRecoveryButton.forceActiveFocus()',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery dialog keyboard markers: " + ", ".join(missing))
    print(f"persisted recovery dialog keyboard contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
