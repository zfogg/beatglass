from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummaryReviewAction"')
block = qml[start:start + 1200]
assert 'current settings revision " + window.overlaySettingsRevision' in block
assert 'Focus the consolidated overlay settings summary' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Accessible.description' in block
assert 'recovery state' in block
print("Overlay summary action description contract verified (5 checks)")
