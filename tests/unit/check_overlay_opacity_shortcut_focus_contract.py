"""Verify keyboard opacity reset returns focus to the opacity control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
block = qml[qml.index('objectName: "overlayOpacityResetShortcut"'):]
block = block[:block.index('Shortcut {', 1)] if 'Shortcut {' in block[1:] else block
if block.count("overlayOpacityControl.forceActiveFocus()") < 2:
    raise SystemExit("Missing opacity shortcut focus restoration")
print("Overlay opacity shortcut focus contract verified (1 check)")
