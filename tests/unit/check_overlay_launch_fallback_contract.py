"""Verify relaunch exposes an explicit native-unavailable fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = [
    "property bool overlayLaunchFallbackActive: false",
    "window.overlayLaunchFallbackActive = true",
    "window.overlayLaunchFallbackActive = false",
    '"launch fallback active"',
    "window.syncOverlaySettingsFromBridge()",
    "marks an “launch fallback active” state when the overlay bridge or display list is unavailable",
]
missing = [marker for marker in required if marker not in qml + docs]
if missing:
    raise SystemExit("Missing overlay launch fallback markers: " + ", ".join(missing))
print(f"Overlay launch fallback contract verified ({len(required)} checks)")
