from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayEstimateAttribution"')
block = qml[start:start + 1000]
assert 'Track and deck metadata are unavailable' in block
assert 'selected combined-output source is retained' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Accessible.role: Accessible.StaticText' in block
assert 'Estimate source: selected system output' in block
print("Overlay attribution retention contract verified (5 checks)")
