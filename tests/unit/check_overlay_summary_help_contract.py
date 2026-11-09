from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayShortcutHelp"')
block = qml[start:start + 1500]
assert 'Ctrl+Alt+Shift+M reviews overlay summary' in block
assert 'reviewing the overlay summary' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Keyboard:' in block
assert 'Ctrl+Alt+Shift+S reviews retained settings' in block
print("Overlay summary help contract verified (5 checks)")
