"""Verify the control window exposes a discoverable restore action after hide."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = [
    'objectName: "restoreOverlay"',
    'window.overlayHiddenByUser ? "Restore hidden overlay" : "Restore overlay"',
    'window.overlayHiddenByUser ? "hidden by user" : "available"',
    'Restore the overlay hidden by the user',
    'Explicit hide/unhide now tracks user intent separately',
]
missing = [marker for marker in required if marker not in qml + docs]
if missing:
    raise SystemExit("Missing hidden-overlay restore control markers: " + ", ".join(missing))
print(f"Overlay hidden-restore control contract verified ({len(required)} checks)")
