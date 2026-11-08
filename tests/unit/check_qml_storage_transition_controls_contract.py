from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ('storageRecovery.status !== "Storage closed"', 'storageRecovery.status.indexOf("Storage error:") !== 0'):
    if text.count(marker) < 2:
        raise SystemExit(f"storage transition guard must gate retry and discard: {marker}")
print("QML storage transition control contract verified")
