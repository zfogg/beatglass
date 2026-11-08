"""Contract checks for selected identity in discard cancellation feedback."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "discardRecoveryDialog"',
        'onRejected: { const runId = storageRecovery.selectedRunId',
        'window.announceVisualizationRecovery("Persisted recovery discard canceled for " + runId + "; selected run was kept.")',
        'discardPersistedRecoveryButton.forceActiveFocus()',
        'onAccepted: { const runId = storageRecovery.selectedRunId',
        'window.announceVisualizationRecovery("Persisted recovery run " + runId + " discarded; selected retry state cleared.")',
        'standardButton(Dialog.Cancel).forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery cancel identity markers: " + ", ".join(missing))
    print(f"persisted recovery discard cancel identity contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
