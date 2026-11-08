"""Verify display settings expose one coherent accessible summary."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsSummary"',
    '"Overlay settings: "',
    'window.overlayBridge.displayName',
    '["Compact", "Medium", "Large"][window.overlayPreferredSizeIndex]',
    'Math.round(window.overlayBridge.opacity * 100)',
    'window.overlayDisplayRecoveryPending ? " · recovery pending"',
    'Accessible.name: "Overlay settings summary"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay settings summary markers: " + ", ".join(missing))
print(f"Overlay settings summary contract verified ({len(required)} checks)")
