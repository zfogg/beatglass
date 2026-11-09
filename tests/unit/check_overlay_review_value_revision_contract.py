from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1200]
assert 'settings revision " + window.overlaySettingsRevision' in block
assert 'recovery pending' in block
assert 'settings retained' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Review retained settings' in block
print("Overlay review value revision contract verified (5 checks)")
