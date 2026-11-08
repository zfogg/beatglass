from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.status === "Retryable run is active"', 'storageRecovery.selectedRunId !== ""', 'discard remains unavailable until it reaches a terminal state'):
    if marker not in text:
        raise SystemExit(f"missing active selected status marker: {marker}")
print("QML active selected recovery status contract verified")
