from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionStatus"')
block = qml[start:start + 1000]
assert 'Accessible.value: text + "; settings revision " + window.overlaySettingsRevision' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Overlay settings retained' in block
assert 'overlaySettingsRevision' in block
print("Overlay retention revision contract verified (5 checks)")
