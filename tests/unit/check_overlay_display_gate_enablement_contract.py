"""Verify Show/Restore/Reapply actions are gated on an explicit display target."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "showOverlay"',
    'objectName: "restoreOverlay"',
    'objectName: "reapplyOverlaySettings"',
    '!window.overlayDisplaySelectionRequired',
    'window.overlayBridge.screenNames.length > 0',
    '"choose a display first"',
    'Choose an available display before showing the overlay',
    'Choose an available display before restoring the overlay',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display gate enablement markers: " + ", ".join(missing))
if qml.count("!window.overlayDisplaySelectionRequired") < 3:
    raise SystemExit("show, restore, and reapply must all honor display selection gating")
print(f"Overlay display gate enablement contract verified ({len(required)} checks)")
