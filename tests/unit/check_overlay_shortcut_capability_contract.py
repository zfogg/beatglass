from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayShortcutHelp"')
block = qml[start:start + 1200]
assert 'Ctrl+Alt+Shift+S reviews retained settings' in block
assert 'retained-settings review shortcut available' in block
assert 'reviewing retained overlay settings' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Keyboard:' in block
print("Overlay shortcut capability contract verified (5 checks)")
