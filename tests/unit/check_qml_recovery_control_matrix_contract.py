from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.analysisStatus !== "Analysis queued"', 'storageRecovery.analysisStatus !== "Analysis running"', 'retry is unavailable until it reaches a terminal state'):
    if marker not in text:
        raise SystemExit(f"missing recovery control matrix marker: {marker}")
print("QML recovery control matrix contract verified")
