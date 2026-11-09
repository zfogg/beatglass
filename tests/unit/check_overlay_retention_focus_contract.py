from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionStatus"')
block = qml[start:start + 1300]
assert 'activeFocusOnTab: true' in block
assert 'Persisted settings remain available' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Overlay settings retained while recovery is pending' in block
assert 'overlaySettingsRetentionStatus' in block
print("Overlay retention focus contract verified (5 checks)")
