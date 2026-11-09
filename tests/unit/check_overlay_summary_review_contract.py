from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummaryReviewAction"')
block = qml[start:start + 900]
assert 'Review overlay summary' in block
assert 'overlaySettingsSummary.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'display, size, opacity, and recovery state' in block
assert 'activeFocusOnTab: true' in block
print("Overlay summary review contract verified (5 checks)")
