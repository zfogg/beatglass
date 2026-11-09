"""Verify the summary description explains each overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsSummary"',
    'Accessible.description: "Current overlay display, size, opacity, and settings revision. "',
    'window.overlayDisplayRecoveryPending',
    '"Recovery is pending; persisted settings are retained while the display is restored."',
    'window.overlayLaunchFallbackActive',
    '"The overlay is using its launch fallback while persisted settings remain available."',
    '"The overlay is ready with persisted settings applied."',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay summary description markers: " + ", ".join(missing))
print(f"Overlay summary description contract verified ({len(required)} checks)")
