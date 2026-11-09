from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "quitBeatglass"')
block = qml[start:start + 1000]
assert 'window.quitApplication()' in block
assert 'Beatglass quitting' in block
assert 'display, geometry, size, opacity' in block
assert 'interaction settings are retained' in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay quit announcement contract verified (5 checks)")
