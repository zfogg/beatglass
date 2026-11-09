from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayShortcutHelp"')
block = qml[start:start + 1200]
assert 'summary review available' in qml
assert 'Accessible.role: Accessible.Status' in block
assert 'Keyboard:' in block
assert 'overlayShortcutHelp' in block
assert 'summary review available' in qml
print("Overlay summary help state contract verified (5 checks)")
