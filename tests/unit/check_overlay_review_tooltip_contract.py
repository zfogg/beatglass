from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1500]
assert 'ToolTip.text: Accessible.description + " Current settings revision: " + window.overlaySettingsRevision' in block
assert 'Accessible.description' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'window.overlaySettingsRevision' in block
assert 'Review retained settings' in block
print("Overlay review tooltip contract verified (5 checks)")
