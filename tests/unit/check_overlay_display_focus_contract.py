from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayDisplay"')
block = qml[start:start + 1400]
assert 'window.persistOverlayDisplaySelection(currentIndex, currentText)' in block
assert 'overlayDisplayStatus.forceActiveFocus()' in block
assert 'forceActiveFocus()' in block
assert 'Accessible.role: Accessible.ComboBox' in block
assert 'current display' in block.lower()
print("Overlay display focus contract verified (5 checks)")
