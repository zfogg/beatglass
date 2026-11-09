from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayOpacity"')
block = qml[start:start + 1500]
assert 'window.overlayBridge.setOpacity(value)' in block
assert 'onPressedChanged: if (!pressed) overlayInteractionMode.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Slider' in block
assert 'Adjust the overlay window opacity' in block
assert 'overlaySettingsRevision' in block
print("Overlay opacity focus contract verified (5 checks)")
