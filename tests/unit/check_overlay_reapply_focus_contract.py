from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "reapplyOverlaySettings"')
block = qml[start:start + 1300]
assert 'window.reapplyOverlaySettingsAndRestore()' in block
assert 'overlayDisplayStatus.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Reapply the saved display' in block
assert 'Current overlay display' in qml
print("Overlay reapply focus contract verified (5 checks)")
