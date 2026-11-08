from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.analysisStatus !== "Analysis queued"', 'storageRecovery.analysisStatus !== "Analysis running"', 'discard is unavailable until terminal resolution'):
    if marker not in text:
        raise SystemExit(f"missing discard matrix marker: {marker}")
print("QML recovery discard matrix contract verified")
