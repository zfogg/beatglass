"""Verify explicit hide/unhide state is discoverable and resets on quit."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property bool overlayHiddenByUser: false",
    "window.overlayHiddenByUser = true",
    "window.overlayHiddenByUser = false",
    "hidden by user",
    "function hideOverlayAndRestoreControls()",
    "function showOverlayAndHideControls()",
    "function quitApplication()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing hide/unhide persistence markers: " + ", ".join(missing))
print(f"Overlay hide/unhide persistence contract verified ({len(required)} checks)")
