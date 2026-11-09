from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "retryOverlay"')
block = qml[start:start + 3200]
assert 'DJ overlay retry requested' in block
assert 'window.showOverlayAndHideControls()' in block
assert 'overlayDisplayStatus.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Select an output display' in block
availability = qml[qml.index('function onAvailableChanged()'):qml.index('function onAvailableChanged()') + 1800]
assert 'retryOverlay.forceActiveFocus()' in availability
assert 'Keep keyboard recovery actionable' in availability
print('Overlay native retry focus contract verified (7 checks)')
