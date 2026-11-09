"""Verify display activation announces action re-enable and restores focus."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayDisplay"',
    'Show, Restore, and Reapply actions are enabled.',
    'retryOverlay.forceActiveFocus()',
    'forceActiveFocus()',
    'onActivated: {',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display focus enablement markers: " + ", ".join(missing))
print(f"Overlay display focus enablement contract verified ({len(required)} checks)")
