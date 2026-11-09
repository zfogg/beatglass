from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayShortcutHelp"')
block = qml[start:start + 1200]
assert 'Ctrl+Alt+Shift+S reviews retained settings' in block
assert 'reviewing retained overlay settings' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Overlay keyboard shortcuts' in block
assert 'Ctrl+Alt+Shift+P focuses reapply' in block
print("Overlay shortcut help contract verified (5 checks)")
