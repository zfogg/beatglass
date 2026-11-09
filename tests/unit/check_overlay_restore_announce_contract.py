from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "restoreOverlay"')
block = qml[start:start + 1600]
assert 'window.showOverlayAndHideControls()' in block
assert 'DJ overlay restore requested' in block
assert 'persisted display, geometry, and opacity' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Restore the overlay' in block
print("Overlay restore announcement contract verified (5 checks)")
