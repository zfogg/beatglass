from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "retryOverlay"')
block = qml[start:start + 1800]
assert 'DJ overlay retry requested' in block
assert 'window.showOverlayAndHideControls()' in block
assert 'overlayDisplayStatus.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Select an output display' in block
print("Overlay native retry focus contract verified (5 checks)")
