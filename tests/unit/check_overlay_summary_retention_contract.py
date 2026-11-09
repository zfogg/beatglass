from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummary"')
block = qml[start:start + 1600]
assert 'recovery pending · settings retained' in block
assert 'launch fallback active' in block
assert 'settings retained' in block
assert 'whether persisted settings are retained' in block
assert 'Accessible.role: Accessible.Status' in block
print("Overlay summary retention contract verified (5 checks)")
