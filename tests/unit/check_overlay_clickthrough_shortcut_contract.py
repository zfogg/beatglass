"""Verify click-through shortcut delegates unlock to the focus-safe helper."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (!requested) {\n                window.unlockOverlayAndRestoreFocus()\n                return\n            }'
if marker not in qml:
    raise SystemExit("Missing click-through shortcut unlock handoff")
print("Overlay click-through shortcut contract verified (1 check)")
