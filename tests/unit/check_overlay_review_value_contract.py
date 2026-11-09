from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1200]
assert 'Accessible.value: window.overlayDisplayRecoveryPending ? "recovery pending" : "settings retained"' in block
assert 'Review recovery settings' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'Review retained settings' in block
print("Overlay review value contract verified (5 checks)")
