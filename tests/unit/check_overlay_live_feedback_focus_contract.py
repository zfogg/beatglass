from pathlib import Path


QML = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = QML.index('objectName: "overlayMonitoringAction"')
end = QML.index('objectName: "overlayCaptureRecoveryAction"', start)
block = QML[start:end]

assert 'overlayLiveMonitoringState' in block
assert 'forceActiveFocus()' in block
assert 'window.announceCaptureRecoveryState' in block

label_start = QML.index('objectName: "overlayLiveMonitoringState"')
label_end = QML.index('objectName: "overlayMonitoringAction"', label_start)
label = QML[label_start:label_end]
assert 'activeFocusOnTab: true' in label
assert 'Accessible.role: Accessible.Status' in label

print("Overlay live feedback focus contract verified (5 checks)")
