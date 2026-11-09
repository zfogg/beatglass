from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayCaptureRecoveryAction"')
end = qml.index('objectName: "overlayEstimateAttribution"', start)
block = qml[start:end]

assert 'Retry overlay capture' in block
assert 'window.captureMonitorBridge.start()' in block
assert 'fresh accepted snapshot' in block
assert 'overlayLiveMonitoringState.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block

print("Overlay capture retry focus contract verified (5 checks)")
