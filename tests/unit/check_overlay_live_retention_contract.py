from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayLiveMonitoringState"')
block = qml[start:start + 1100]
assert 'selected endpoint remains retained' in block
assert 'monitored combined output' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Accessible.role: Accessible.Status' in block
assert 'Overlay monitoring:' in block
print("Overlay live retention contract verified (5 checks)")
