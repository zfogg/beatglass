from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('objectName: "discardAllPersistedRecovery"', "storageRecovery.discardRuns(storageRecovery.retryableRunIds)", "All saved recovery runs discarded"):
    if marker not in text:
        raise SystemExit(f"missing QML batch discard marker: {marker}")
print("QML batch discard action contract verified")
