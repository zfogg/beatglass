from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1600]
assert 'settings revision " + window.overlaySettingsRevision' in block
assert 'Reviewing retained overlay settings while recovery is pending' in block
assert 'native overlay recovery is available at settings revision' in block
assert 'window.announceCaptureRecoveryState' in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay review revision contract verified (5 checks)")
