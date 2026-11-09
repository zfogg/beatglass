from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySize"')
block = qml[start:start + 1800]
assert 'window.overlayBridge.resizeOverlay' in block
assert 'overlayGeometryStatus.forceActiveFocus()' in block
assert 'window.announceCaptureRecoveryState' in block
assert 'Accessible.role: Accessible.ComboBox' in block
assert 'window.overlayPreferredSizeIndex' in block
print("Overlay size focus contract verified (5 checks)")
