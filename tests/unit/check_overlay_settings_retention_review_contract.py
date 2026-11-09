from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 900]
assert 'Review retained settings' in block
assert 'overlaySettingsRetentionStatus.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'persisted display, geometry, opacity, and interaction settings' in block
assert 'activeFocusOnTab: true' in block
print("Overlay retention review action contract verified (5 checks)")
