from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "hideOverlay"')
block = qml[start:start + 900]
assert 'window.hideOverlayAndRestoreControls()' in block
assert 'restoreOverlay.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'restore the control window' in block
assert 'Hide the DJ overlay' in block
print("Overlay hide focus contract verified (5 checks)")
