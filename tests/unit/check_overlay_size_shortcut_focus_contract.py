"""Verify keyboard size cycling returns focus to the size control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
block = qml[qml.index('objectName: "overlaySizeShortcut"'):]
block = block[:block.index('Shortcut {', 1)] if 'Shortcut {' in block[1:] else block
if block.count("overlaySize.forceActiveFocus()") < 2:
    raise SystemExit("Missing size shortcut focus restoration")
print("Overlay size shortcut focus contract verified (1 check)")
