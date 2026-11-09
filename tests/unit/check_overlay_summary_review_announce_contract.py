from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummaryReviewAction"')
block = qml[start:start + 1200]
assert 'overlaySettingsSummary.forceActiveFocus()' in block
assert 'Overlay summary reviewed; settings revision' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'window.announceCaptureRecoveryState' in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay summary review announcement contract verified (5 checks)")
