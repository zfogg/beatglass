from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayEndpointStatus"')
block = qml[start:start + 1500]
assert 'selected endpoint remains retained' in block
assert 'last endpoint selection remains retained' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'selected system output endpoint' in block
print("Overlay endpoint retention contract verified (5 checks)")
