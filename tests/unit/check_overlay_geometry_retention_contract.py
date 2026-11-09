from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayGeometryStatus"')
block = qml[start:start + 1200]
assert 'settings are retained' in block
assert 'Recovery is pending' in block
assert 'control-window fallback remains active' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Persisted overlay position and size' in block
print("Overlay geometry retention contract verified (5 checks)")
