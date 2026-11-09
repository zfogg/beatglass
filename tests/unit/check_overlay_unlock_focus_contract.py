from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "unlockOverlay"')
block = qml[start:start + 1000]
assert 'window.unlockOverlayAndRestoreFocus()' in block
assert 'overlayInteractionMode.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Restore keyboard and pointer access' in block
assert 'click-through' in block
print("Overlay unlock focus contract verified (5 checks)")
