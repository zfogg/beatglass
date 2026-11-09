from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "showOverlay"')
block = qml[start:start + 1500]
assert 'window.showOverlayAndHideControls()' in block
assert 'DJ overlay launch requested' in block
assert 'persisted settings' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'selected display' in block
print("Overlay show announcement contract verified (5 checks)")
