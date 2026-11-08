from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('objectName: "discardAllRecoveryDialog"', "standardButtons: Dialog.Ok | Dialog.Cancel", "onAccepted: { storageRecovery.discardRuns(storageRecovery.retryableRunIds)", "onRejected: discardAllPersistedRecovery.forceActiveFocus()"):
    if marker not in text:
        raise SystemExit(f"missing batch discard confirmation marker: {marker}")
print("QML batch discard confirmation contract verified")
