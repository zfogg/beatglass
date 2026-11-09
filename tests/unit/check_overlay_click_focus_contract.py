from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "clickThrough"')
block = qml[start:start + 1400]
assert 'window.overlayBridge.setClickThrough(requestedClickThrough)' in block
assert 'overlayInteractionMode.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.CheckBox' in block
assert 'could not change input mode' in block
assert 'Toggle pointer click-through' in block
print("Overlay click-through focus contract verified (5 checks)")
