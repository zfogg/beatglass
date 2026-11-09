from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1500]
assert 'current settings revision " + window.overlaySettingsRevision' in block
assert 'recovery is pending' in block
assert 'persisted display, geometry, opacity, and interaction settings' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Accessible.description' in block
print("Overlay review description contract verified (5 checks)")
