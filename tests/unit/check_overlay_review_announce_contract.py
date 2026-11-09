from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1800]
assert 'overlaySettingsRetentionStatus.forceActiveFocus()' in block
assert 'Reviewing retained overlay settings while recovery is pending.' in block
assert 'Reviewing retained overlay settings; native overlay recovery is available.' in block
assert 'window.announceCaptureRecoveryState' in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay review announcement contract verified (5 checks)")
