from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('retryable recovery runs discarded', 'storageRecovery.status.indexOf'):
    if marker not in text:
        raise SystemExit(f"missing QML batch result status marker: {marker}")
print("QML batch result status contract verified")
