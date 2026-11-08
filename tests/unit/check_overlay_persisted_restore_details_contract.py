"""Verify persisted restore status exposes display, size, opacity, and recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayRestoreState"',
    '"Persisted restore: "',
    'window.overlayBridge.displayName',
    '["Compact", "Medium", "Large"][window.overlayPreferredSizeIndex]',
    'Math.round(window.overlayBridge.opacity * 100)',
    'window.overlayDisplayRecoveryPending ? " · recovery pending"',
    'Accessible.description: "Restored overlay display and opacity',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing persisted restore detail markers: " + ", ".join(missing))
print(f"Overlay persisted restore details contract verified ({len(required)} checks)")
