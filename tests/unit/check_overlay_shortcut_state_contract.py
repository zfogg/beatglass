from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayShortcutHelp"')
block = qml[start:start + 1700]
assert 'retained-settings review shortcut available' in block
assert 'retained-settings review unavailable' in block
assert 'window.overlayBridge !== null' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Ctrl+Alt+Shift+S reviews retained settings' in block
print("Overlay shortcut state contract verified (5 checks)")
