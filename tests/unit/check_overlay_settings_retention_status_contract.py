from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionStatus"')
block = qml[start:start + 1200]
assert 'Overlay settings retained while recovery is pending' in block
assert 'display, geometry, opacity, and interaction mode' in block
assert 'Persisted settings remain available' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'window.overlayDisplayRecoveryPending' in block
print("Overlay settings retention status contract verified (5 checks)")
