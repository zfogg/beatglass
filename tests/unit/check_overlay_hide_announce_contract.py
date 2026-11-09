from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "hideOverlay"')
block = qml[start:start + 1200]
assert 'window.hideOverlayAndRestoreControls()' in block
assert 'DJ overlay hidden' in block
assert 'control-window fallback restored' in block
assert 'overlay settings retained' in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay hide announcement contract verified (5 checks)")
