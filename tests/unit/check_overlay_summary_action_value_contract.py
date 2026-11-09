from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummaryReviewAction"')
block = qml[start:start + 1100]
assert 'Accessible.value: (window.overlayDisplayRecoveryPending ? "recovery pending" : "available") + "; settings revision " + window.overlaySettingsRevision' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Review overlay summary' in block
assert 'window.overlaySettingsRevision' in block
print("Overlay summary action value contract verified (5 checks)")
