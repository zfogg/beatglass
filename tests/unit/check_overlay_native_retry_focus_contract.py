from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "retryOverlay"')
block = qml[start:start + 3200]
assert 'DJ overlay retry requested' in block
assert 'window.showOverlayAndHideControls()' in block
assert 'overlayDisplayStatus.forceActiveFocus()' in block
assert 'Accessible.role: Accessible.Button' in block
assert 'Select an output display' in block
availability = qml[qml.index('function onAvailableChanged()'):qml.index('function onAvailableChanged()') + 1800]
assert 'retryOverlay.forceActiveFocus()' in availability
assert 'Keep keyboard recovery actionable' in availability
qml_tests = Path('tests/qml/tst_navigation.qml').read_text(encoding='utf-8')
assert 'test_validVisualRestoreIsDirectlyTestable' in qml_tests
assert '42 percent opacity' in qml_tests
docs = Path('docs/issue-76-visual-implementation-audit.md').read_text(encoding='utf-8')
assert 'valid persisted visual-state restore path' in docs
print('Overlay native retry focus contract verified (10 checks)')
