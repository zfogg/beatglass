from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.analysisStatus !== "Analysis complete"', 'selected analysis is complete; no persisted retry record can be discarded'):
    if marker not in text:
        raise SystemExit(f"missing terminal control state marker: {marker}")
print("QML terminal control state contract verified")
