"""Contract checks for safe action guidance in persisted recovery discard."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'standardButtons: Dialog.Ok | Dialog.Cancel',
        'Accessible.description: "Confirm to remove only the selected retry record. Cancel is focused first; press Tab to reach the destructive OK action.',
        'standardButton(Dialog.Cancel).forceActiveFocus()',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
        'discardPersistedRecoveryButton.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery action guidance markers: " + ", ".join(missing))
    print(f"persisted recovery dialog action guidance contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
