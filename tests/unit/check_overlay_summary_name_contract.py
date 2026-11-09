from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummary"')
block = qml[start:start + 1600]
assert 'Overlay settings summary, ' in block
assert 'recovery pending' in block
assert '"ready"' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'settings revision' in block
print("Overlay summary name contract verified (5 checks)")
