"""Verify reapply shortcut returns focus to its action control."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
block = qml[qml.index('objectName: "overlayReapplyShortcut"'):]
block = block[:block.index('Shortcut {', 1)] if 'Shortcut {' in block[1:] else block
if "reapplyOverlaySettings.forceActiveFocus()" not in block:
    raise SystemExit("Missing reapply shortcut focus restoration")
print("Overlay reapply shortcut focus contract verified (1 check)")
