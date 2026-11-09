from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySummaryReviewShortcut"')
block = qml[start:start + 700]
assert 'sequence: "Ctrl+Alt+Shift+M"' in block
assert 'overlaySettingsSummaryReviewAction.forceActiveFocus()' in block
assert 'navigation.currentIndex = 1' in block
assert 'Overlay summary review control focused.' in block
assert 'context: Qt.ApplicationShortcut' in block
print("Overlay summary shortcut contract verified (5 checks)")
