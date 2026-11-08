"""Contract checks for safe initial focus in persisted recovery confirmation."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'standardButtons: Dialog.Ok | Dialog.Cancel',
        'onOpened: { window.announceVisualizationRecovery("Confirm discard for persisted recovery run " + storageRecovery.selectedRunId',
        'standardButton(Dialog.Cancel).forceActiveFocus()',
        'Accessible.description: "Confirm to remove only the selected retry record.',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
        'discardPersistedRecoveryButton.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery safe-focus markers: " + ", ".join(missing))
    print(f"persisted recovery dialog safe-focus contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
