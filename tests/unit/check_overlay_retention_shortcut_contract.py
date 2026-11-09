from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayRetentionFocusShortcut"')
block = qml[start:start + 700]
assert 'sequence: "Ctrl+Alt+Shift+S"' in block
assert 'overlaySettingsRetentionReviewAction.forceActiveFocus()' in block
assert 'navigation.currentIndex = 1' in block
assert 'Retained overlay settings review control focused.' in block
assert 'context: Qt.ApplicationShortcut' in block
print("Overlay retention shortcut contract verified (5 checks)")
