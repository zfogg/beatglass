from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.analysisStatus === "Analysis complete"', 'storageRecovery.analysisStatus === "Analysis cancelled"', 'persisted recovery remains retryable'):
    if marker not in text:
        raise SystemExit(f"missing terminal recovery UX marker: {marker}")
print("QML terminal recovery status contract verified")
