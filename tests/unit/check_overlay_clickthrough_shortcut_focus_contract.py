"""Verify click-through shortcut returns focus to its control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
block = qml[qml.index('objectName: "overlayClickThroughShortcut"'):]
block = block[:block.index('Shortcut {', 1)] if 'Shortcut {' in block[1:] else block
if block.count("clickThroughToggle.forceActiveFocus()") < 2:
    raise SystemExit("Missing click-through shortcut focus restoration")
print("Overlay click-through shortcut focus contract verified (1 check)")
