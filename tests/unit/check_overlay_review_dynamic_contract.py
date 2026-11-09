from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsRetentionReviewAction"')
block = qml[start:start + 1100]
assert 'Review recovery settings' in block
assert 'Review retained settings' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'pending-recovery guidance' not in block
assert 'Accessible.role: Accessible.Button' in block
print("Overlay dynamic review contract verified (5 checks)")
