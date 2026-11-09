"""Verify the overlay summary tooltip communicates recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsSummary"',
    'ToolTip.text: window.overlayDisplayRecoveryPending',
    '"Overlay recovery pending; persisted display settings are retained"',
    'window.overlayLaunchFallbackActive',
    '"Overlay launch fallback active; persisted display settings are available"',
    '"Current overlay display, size, opacity, and recovery state"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay summary tooltip markers: " + ", ".join(missing))
print(f"Overlay summary tooltip contract verified ({len(required)} checks)")
