from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayRecoveryStep"')
end = qml.index('objectName: "overlayGeometryStatus"', start)
block = qml[start:end]

assert "window.overlayDisplayRecoveryPending" in block
assert "Review settings" in block
assert "Reconnect a display" in block
assert "Retry overlay" in block
print("overlay recovery step UI contract: ok")
