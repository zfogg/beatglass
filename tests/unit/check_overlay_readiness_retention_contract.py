from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayDataReadiness"')
block = qml[start:start + 1100]
assert 'accepted snapshot remains retained' in block
assert 'last accepted snapshot remains retained' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Overlay estimates: accepted snapshot ready' in block
print("Overlay readiness retention contract verified (5 checks)")
