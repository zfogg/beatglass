from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySummaryReviewShortcut"')
block = qml[start:start + 800]
assert 'settings revision " + window.overlaySettingsRevision' in block
assert 'window.overlayDisplayRecoveryPending' in block
assert 'overlaySummaryReviewShortcut' in block
assert 'navigation.currentIndex = 1' in block
assert 'Qt.ApplicationShortcut' in block
print("Overlay summary shortcut revision contract verified (5 checks)")
